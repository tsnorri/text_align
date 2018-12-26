# Copyright (c) 2018 Tuukka Norri
# This code is licensed under MIT license (see LICENSE for details).

from flask import Flask, request, jsonify, abort
import json
import re
from werkzeug.exceptions import HTTPException

import sys
sys.path.append("../python")
import text_align


app = Flask(__name__, static_folder = 'site', static_url_path = '')


def abort_on_exception(code, message):
	"""Log the exception traceback and pass the error message to the client."""
	app.logger.error("Calling abort(%d) on exception '%s'" % (code, message), exc_info = True)
	abort(code, {'message': message})


def abort_with_message(code, message):
	"""Pass the error message to the client."""
	abort(code, {'message': message})


def decodeFileContents(request, key):
	"""Create Unicode strings from the file contents."""
	if key not in request.files:
		abort_with_message(400, "File for key '%s' not given" % key)
	
	try:
		retval = request.files[key].read().decode("UTF-8")
		return retval
	except Exception as error:
		abort_on_exception(400, "Unable to read UTF-8 from '%s'" % key)


def processAlignerInput(request, ctx):
	"""Process the sent data."""
	content_type = request.headers.get('Content-Type')
	if 'application/json' == content_type:
		content = request.get_json()
		
		# Configure the aligner.
		ctx.lhs = content['left']
		ctx.rhs = content['right']
		ctx.identity_score = content['match']
		ctx.mismatch_penalty = content['mismatch']
		ctx.gap_start_penalty = content['gap_start']
		ctx.gap_penalty = content['gap']
		
		should_return_alignment_graph = content['create_alignment_graph']
		if should_return_alignment_graph:
			ctx.setup_bit_vectors()
		else:
			ctx.setup_run_vectors()
		
		return should_return_alignment_graph
		
	elif re.match(r"^multipart/form-data[$;]", content_type):
		lhs = decodeFileContents(request, 'left')
		rhs = decodeFileContents(request, 'right')
		
		# Configure the aligner.
		ctx.lhs = lhs
		ctx.rhs = rhs
		ctx.identity_score = int(request.form.get('match'))
		ctx.mismatch_penalty = int(request.form.get('mismatch'))
		ctx.gap_start_penalty = int(request.form.get('gap-start'))
		ctx.gap_penalty = int(request.form.get('gap'))
		ctx.setup_bit_vectors()
		
		return True
	else:
		abort_with_message(400, "Unable to handle Content-Type '%s'" % content_type)


@app.errorhandler(400)
def custom400(error):
	return json.dumps(error.description), 400, {'Content-Type': 'application/json'}


@app.route('/')
def root():
	return app.send_static_file('index.html')


@app.route('/align', methods = ['POST'])
def align():
	# Convert the files’ contents to Unicode strings.
	try:
		ctx = text_align.SmithWatermanAlignmentContext()
		shouldCreateAlignmentGraph = processAlignerInput(request, ctx)
		
		# Run the aligner.
		ctx.align()
		
		if shouldCreateAlignmentGraph:
			# Make an alignment graph and return it as JSON.
			nodes = ctx.make_alignment_graph()
			nodes = [x.toJSON() for x in nodes]
			return jsonify(nodes)
		else:
			# Output bit vectors packaged in JSON.
			lhs_starts_with_zero, lhs_runs = ctx.make_lhs_runs()
			rhs_starts_with_zero, rhs_runs = ctx.make_rhs_runs()
			retval = {
				"left":		{ "starts_with_zero": lhs_starts_with_zero, "runs": lhs_runs },
				"right":	{ "starts_with_zero": rhs_starts_with_zero, "runs": rhs_runs },
			}
			return jsonify(retval)
			
	except HTTPException as error:
		# abort() already called.
		raise error
		
	except Exception as error:
		abort_on_exception(400, str(error))


if __name__ == "__main__":
	app.run(port = 5002)
