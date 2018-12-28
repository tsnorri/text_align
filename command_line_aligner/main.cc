/*
 * Copyright (c) 2018 Tuukka Norri
 * This code is licensed under MIT license (see LICENSE for details).
 */

#include <libbio/algorithm.hh>

#include <boost/dynamic_bitset.hpp>
#include <boost/locale/utf.hpp>
#include <boost/range.hpp>
#include <iostream>
#include <libbio/int_vector.hh>
#include <libbio/map_on_stack.hh>
#include <libbio/mmap_handle.hh>
#include <string>
#include <text_align/code_point_iterator.hh>
#include <text_align/smith_waterman/aligner.hh>
#include <text_align/smith_waterman/alignment_context.hh>
#include <vector>

#include "cmdline.h"


namespace ta = text_align;


typedef std::int32_t score_type;
enum { SCORE_MAX = std::numeric_limits <score_type>::max() };

template <typename t_word, typename t_delegate>
using aligner_type = ta::smith_waterman::aligner <score_type, t_word, t_delegate>;

typedef ta::smith_waterman::detail::score_result <score_type> score_result;


struct score_container
{
	libbio::matrix <score_type>		score;
	libbio::matrix <score_type>		gap_score_lhs;
	libbio::matrix <score_type>		gap_score_rhs;
	libbio::matrix <std::uint8_t>	max_idx;
	libbio::matrix <std::uint8_t>	did_start_gap;
	
	score_container() = default;
	
	void resize(std::size_t const rows, std::size_t const columns)
	{
		score.resize(rows, columns);
		gap_score_lhs.resize(rows, columns);
		gap_score_rhs.resize(rows, columns);
		max_idx.resize(rows, columns);
		did_start_gap.resize(rows, columns);
		
		std::fill(score.begin(), score.end(), SCORE_MAX);
		std::fill(gap_score_lhs.begin(), gap_score_lhs.end(), SCORE_MAX);
		std::fill(gap_score_rhs.begin(), gap_score_rhs.end(), SCORE_MAX);
		std::fill(max_idx.begin(), max_idx.end(), UINT8_MAX);
		std::fill(did_start_gap.begin(), did_start_gap.end(), UINT8_MAX);
	}
};


// Call boost::asio::io_context::stop when needed.
class aligner_delegate
{
protected:
	boost::asio::io_context	*m_pool{};
	libbio::bit_vector m_lhs_gaps;
	libbio::bit_vector m_rhs_gaps;
	
public:
	aligner_delegate() = default;
	aligner_delegate(boost::asio::io_context &pool):
		m_pool(&pool)
	{
	}
	
	libbio::bit_vector const &lhs_gaps() const { return m_lhs_gaps; }
	libbio::bit_vector const &rhs_gaps() const { return m_rhs_gaps; }
	
	virtual void will_run_aligner(ta::smith_waterman::aligner_base &aligner, std::size_t const lhs_size, std::size_t const rhs_size) {}
	void finish(ta::smith_waterman::aligner_base &aligner) { /* m_pool->stop(); */ }
	
	void push_lhs(bool flag, std::size_t count) { m_lhs_gaps.push_back(flag, count); }
	void push_rhs(bool flag, std::size_t count) { m_rhs_gaps.push_back(flag, count); }
	void clear_gaps() { m_lhs_gaps.clear(); m_rhs_gaps.clear(); }
	void reverse_gaps() { m_lhs_gaps.reverse(); m_rhs_gaps.reverse(); }
};


// Store the alignment scores for later comparison.
class verifying_aligner_delegate final : public aligner_delegate
{
protected:
	score_container	m_scores;
	
public:
	using aligner_delegate::aligner_delegate;
	
	score_container const &scores() const { return m_scores; }
	
	virtual void will_run_aligner(ta::smith_waterman::aligner_base &aligner, std::size_t const lhs_size, std::size_t const rhs_size) override
	{
		// Reset the block size s.t. just one block will be filled.
		aligner.set_segment_length(1 + std::max(lhs_size, rhs_size));
		
		// Reserve memory for the scores.
		m_scores.resize(1 + lhs_size, 1 + rhs_size);
	}
	
	void did_calculate_score(ta::smith_waterman::aligner_base &aligner, std::size_t j, std::size_t i, score_result const &result, bool initial)
	{
		if (initial)
		{
			// Store the calculated values.
			libbio_assert(SCORE_MAX == m_scores.score(j, i));
			libbio_assert(SCORE_MAX == m_scores.gap_score_lhs(j, i));
			libbio_assert(SCORE_MAX == m_scores.gap_score_rhs(j, i));
			libbio_assert(UINT8_MAX == m_scores.max_idx(j, i));
			libbio_assert(UINT8_MAX == m_scores.did_start_gap(j, i));
			
			m_scores.score(j, i) = result.score;
			m_scores.gap_score_lhs(j, i) = result.gap_score_lhs;
			m_scores.gap_score_rhs(j, i) = result.gap_score_rhs;
			m_scores.max_idx(j, i) = result.max_idx;
			m_scores.did_start_gap(j, i) = result.did_start_gap;
		}
		else
		{
			libbio_assert(m_scores.score(j, i) == result.score);
			libbio_assert(m_scores.gap_score_lhs(j, i) == result.gap_score_lhs);
			libbio_assert(m_scores.gap_score_rhs(j, i) == result.gap_score_rhs);
			libbio_assert(m_scores.max_idx(j, i) == result.max_idx);
			libbio_assert(m_scores.did_start_gap(j, i) == result.did_start_gap);
		}
	}
};


// Check that the alignment score matches the previous value.
class tested_aligner_delegate final : public aligner_delegate
{
protected:
	score_container const *m_scores{};
	
public:
	using aligner_delegate::aligner_delegate;
	
	void set_scores(score_container const &container) { m_scores = &container; }
	
protected:
	template <typename t_type>
	void compare(char const *name, std::size_t const j, std::size_t const i, t_type const expected, t_type const actual)
	{
		if (actual != expected)
			std::cerr << name << " mismatch at (" << j << ", " << i << "): expected " << +expected << ", actual " << +actual << ".\n";
	}

public:
	void did_calculate_score(ta::smith_waterman::aligner_base &aligner, std::size_t j, std::size_t i, score_result const &result, bool initial)
	{
		// Compare to the pre-calculated.
		compare("score:        ", j, i, m_scores->score(j, i), result.score);
		compare("gap_score_lhs:", j, i, m_scores->gap_score_lhs(j, i), result.gap_score_lhs);
		compare("gap_score_rhs:", j, i, m_scores->gap_score_rhs(j, i), result.gap_score_rhs);
		compare("max_idx:      ", j, i, m_scores->max_idx(j, i), result.max_idx);
		compare("did_start_gap:", j, i, m_scores->did_start_gap(j, i), static_cast <std::uint8_t>(result.did_start_gap));
	}
};


template <typename t_range>
std::size_t copy_distance(t_range range)
{
	return boost::distance(range);
}


template <typename t_range>
void print_aligned(t_range const &range, libbio::bit_vector const &gaps)
{
	//libbio_assert(str.size() <= gaps.size());
	
	std::string buffer;
	
	auto it(range.begin()), end(range.end());
	auto ostream_it(std::ostream_iterator <char>(std::cout, ""));
	// std::ostream_iterator's operator++ is a no-op, hence we don't
	// save the output value from utf_traits nor use it in else.
	for (auto const val : gaps)
	{
		if (0 == val)
		{
			libbio_assert(it != end);
			boost::locale::utf::utf_traits <char>::encode(*it++, ostream_it);
		}
		else
		{
			std::cout << '-';
		}
	}
	libbio_assert(it == end);
	std::cout << std::endl;
}


struct string_view_from_input
{
	template <typename t_tuple, typename t_fn>
	void operator()(t_tuple const &tuple, t_fn &&fn)
	{
		auto const *arg(std::get <0>(tuple));
		auto const *path_arg(std::get <1>(tuple));
		
		if (arg)
		{
			std::string_view view(arg, strlen(arg));
			fn(view);
		}
		else
		{
			libbio::mmap_handle handle;
			handle.open(path_arg);
			std::string_view view(handle.data(), handle.size());
			fn(view);
		}
	}
};


template <typename t_aligner>
void configure_aligner(t_aligner &aligner, gengetopt_args_info const &args_info)
{
	aligner.set_segment_length(args_info.block_size_arg);
	aligner.set_identity_score(args_info.match_score_arg);
	aligner.set_mismatch_penalty(args_info.mismatch_penalty_arg);
	aligner.set_gap_start_penalty(args_info.gap_start_penalty_arg);
	aligner.set_gap_penalty(args_info.gap_penalty_arg);
	aligner.set_prints_debugging_information(args_info.print_debugging_information_flag);
}


template <typename t_aligner>
void run_aligner(
	t_aligner &aligner,
	typename t_aligner::delegate_type &delegate,
	boost::asio::io_context &pool,
	std::string_view const &lhsv,
	std::string_view const &rhsv,
	gengetopt_args_info const &args_info
)
{
	pool.restart();
	if (args_info.align_bytes_flag)
	{
		auto const lhs_len(lhsv.size());
		auto const rhs_len(lhsv.size());
		delegate.will_run_aligner(aligner, lhs_len, rhs_len);
		aligner.align(lhsv, rhsv, lhs_len, rhs_len);
		pool.run();
		std::cout << "Score: " << aligner.alignment_score() << std::endl;
	}
	else
	{
		auto const lhsr(ta::make_code_point_iterator_range(lhsv.cbegin(), lhsv.cend()));
		auto const rhsr(ta::make_code_point_iterator_range(rhsv.cbegin(), rhsv.cend()));
		auto const lhs_len(copy_distance(lhsr));
		auto const rhs_len(copy_distance(rhsr));
		libbio_assert(lhsr.begin() != lhsr.end());
		libbio_assert(rhsr.begin() != rhsr.end());
		
		delegate.will_run_aligner(aligner, lhs_len, rhs_len);
		aligner.align(lhsr, rhsr, lhs_len, rhs_len);
		pool.run();
		std::cout << "Score: " << aligner.alignment_score() << std::endl;
		print_aligned(lhsr, delegate.lhs_gaps());
		print_aligned(rhsr, delegate.rhs_gaps());
	}
}


void process_input(boost::asio::io_context &pool, gengetopt_args_info const &args_info)
{
	auto const lhs_input(std::make_tuple(args_info.lhs_arg, args_info.lhs_file_arg));
	auto const rhs_input(std::make_tuple(args_info.rhs_arg, args_info.rhs_file_arg));
	libbio::map_on_stack_fn <string_view_from_input>(
		[&pool, &args_info](std::string_view const &lhsv, std::string_view const &rhsv) {
			
			// Check whether the alignment should also be verified.
			if (args_info.verify_alignment_flag)
			{
				// Run first the verifying aligner, then compare the results.
				typedef aligner_type <std::uint64_t, verifying_aligner_delegate> verifying_aligner_type;
				typedef aligner_type <std::uint16_t, tested_aligner_delegate> tested_aligner_type;
				
				verifying_aligner_delegate ad(pool);
				tested_aligner_delegate td(pool);
				td.set_scores(ad.scores());
				
				{
					verifying_aligner_type verifying_aligner(pool, ad);
					configure_aligner(verifying_aligner, args_info);
					run_aligner(verifying_aligner, ad, pool, lhsv, rhsv, args_info);
					libbio_assert(ad.lhs_gaps().size() == ad.rhs_gaps().size());
				}
				
				tested_aligner_type tested_aligner(pool, td);
				configure_aligner(tested_aligner, args_info);
				run_aligner(tested_aligner, td, pool, lhsv, rhsv, args_info);
				
				{
					// Check the gap vectors.
					auto const &lhs_expected_gaps(ad.lhs_gaps());
					auto const &rhs_expected_gaps(ad.rhs_gaps());
					auto const &lhs_gaps(td.lhs_gaps());
					auto const &rhs_gaps(td.rhs_gaps());
					libbio_assert(lhs_gaps.size() == rhs_gaps.size());
					if (lhs_expected_gaps.size() != lhs_gaps.size())
						std::cerr << "Traceback lengths do not match. Expected " << lhs_expected_gaps.size() << ", got " << lhs_gaps.size() << ".\n";
					
					{
						std::cerr << '\n';
						std::size_t i(lhs_gaps.size());
						std::size_t j(lhs_expected_gaps.size());
						while (i && j)
						{
							--i;
							--j;
							if (lhs_gaps[i] != lhs_expected_gaps[j])
								std::cerr << "Mismatch in lhs gaps at positions " << j << " (expected), " << i << " (actual).\n";
						}
					}
					
					{
						std::cerr << '\n';
						std::size_t i(rhs_gaps.size());
						std::size_t j(rhs_expected_gaps.size());
						while (i && j)
						{
							--i;
							--j;
							if (rhs_gaps[i] != rhs_expected_gaps[j])
								std::cerr << "Mismatch in rhs gaps at positions " << j << " (expected), " << i << " (actual).\n";
						}
					}
				}
			}
			else
			{
				aligner_delegate ad(pool);
				aligner_type <std::uint64_t, aligner_delegate> aligner(pool, ad);
				configure_aligner(aligner, args_info);
				run_aligner(aligner, ad, pool, lhsv, rhsv, args_info);
			}
		},
		lhs_input, rhs_input
	);
}


int main(int argc, char **argv)
{
	//std::string lhs("ACTAAGGCTCTCTACCCCTCTCAGAGA");
	//std::string rhs("ACTAAGGCTCCTAACCCCCTTTTCTCAGA");
	
	gengetopt_args_info args_info;
	if (0 != cmdline_parser(argc, argv, &args_info))
		exit(EXIT_FAILURE);
	
	std::ios_base::sync_with_stdio(false);	// Don't use C style IO after calling cmdline_parser.
	std::cin.tie(nullptr);					// We don't require any input from the user.
	
	if (args_info.print_invocation_flag)
	{
		std::cerr << "Invocation:";
		for (int i(0); i < argc; ++i)
			std::cerr << ' ' << argv[i];
		std::cerr << '\n';
	}
	
	if (args_info.block_size_arg < 0)
	{
		std::cerr << "Block size needs to be non-negative." << std::endl;
		exit(EXIT_FAILURE);
	}
	
	if (args_info.single_threaded_flag)
	{
		boost::asio::io_context pool(1);
		process_input(pool, args_info);
	}
	else
	{
		boost::asio::io_context pool;
		process_input(pool, args_info);
	}
	
	cmdline_parser_free(&args_info);
	
	return 0;
}
