/*
 * Copyright (c) 2019 Tuukka Norri
 * This code is licensed under MIT license (see LICENSE for details).
 */

#ifndef TEXT_ALIGN_PY_OBJECT_POINTER_HH
#define TEXT_ALIGN_PY_OBJECT_POINTER_HH

#include <Python.h>

namespace text_align {
	
	class py_object_ptr
	{
	protected:
		PyObject *m_ptr{};
		
	public:
		py_object_ptr() = default;
		
		// Constructor, takes ownership.
		py_object_ptr(PyObject *ptr):
			m_ptr(ptr)
		{
		}
		
		// Constructor, takes ownership conditionally.
		py_object_ptr(PyObject *ptr, bool should_incref):
			m_ptr(ptr)
		{
			libbio_assert(m_ptr);
			if (should_incref)
				Py_INCREF(m_ptr);
		}
		
		// Copy constructor.
		py_object_ptr(py_object_ptr const &other):
			m_ptr(other.m_ptr)
		{
			Py_XINCREF(m_ptr);
		}
		
		// Move constructor.
		// May be default b.c. m_ptr will not have a value before assignment.
		py_object_ptr(py_object_ptr &&other) = default;
		
		// Copy assignment operator.
		py_object_ptr &operator=(py_object_ptr const &other)
		{
			if (this != &other)
			{
				Py_XDECREF(m_ptr);
				m_ptr = other.m_ptr;
				Py_XINCREF(m_ptr);
			}
			
			return *this;
		}
		
		// Move assignment operator.
		// m_ptr may have been set earlier, so use Py_XDECREF.
		py_object_ptr &operator=(py_object_ptr &&other) &
		{
			if (this != &other)
			{
				Py_XDECREF(m_ptr);
				m_ptr = other.m_ptr;
			}
			
			return *this;
		}
		
		// Destructor.
		~py_object_ptr()
		{
			Py_XDECREF(m_ptr);
			m_ptr = nullptr;
		}
		
		// Takes ownership.
		void reset(PyObject *ptr)
		{
			Py_XDECREF(m_ptr);
			m_ptr = ptr;
		}
		
		// Takes ownership conditionally.
		void reset(PyObject *ptr, bool should_incref)
		{
			libbio_assert(ptr);
			Py_XDECREF(m_ptr);
			m_ptr = ptr;
			if (should_incref)
				Py_XINCREF(m_ptr);
		}
		
		// Returns a borrowed reference.
		PyObject *get() const { return m_ptr; }
		
		// Returns a borrowed reference.
		// Not similar to std::unique_ptr since Python’s functions take a pointer.
		PyObject *operator*() const { return m_ptr; }
		
		// Similar to std::unique_ptr’s operator bool.
		operator bool() const { return m_ptr; }
		bool operator==(py_object_ptr const &other) const { return m_ptr == other.m_ptr; }
	};
}

#endif
