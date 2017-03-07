#include "VertexArray.hpp"

#include "Error.hpp"
#include "InvalidObject.hpp"
#include "Primitive.hpp"
#include "Buffer.hpp"

// TODO: implement VertexAttrib as VertexArray member

PyObject * MGLVertexArray_tp_new(PyTypeObject * type, PyObject * args, PyObject * kwargs) {
	MGLVertexArray * self = (MGLVertexArray *)type->tp_alloc(type, 0);

	#ifdef MGL_VERBOSE
	printf("MGLVertexArray_tp_new %p\n", self);
	#endif

	if (self) {
		self->program = 0;
		self->content = 0;
		self->index_buffer = 0;
	}

	return (PyObject *)self;
}

void MGLVertexArray_tp_dealloc(MGLVertexArray * self) {

	#ifdef MGL_VERBOSE
	printf("MGLVertexArray_tp_dealloc %p\n", self);
	#endif

	MGLVertexArray_Type.tp_free((PyObject *)self);
}

int MGLVertexArray_tp_init(MGLVertexArray * self, PyObject * args, PyObject * kwargs) {
	return -1;
}

PyObject * MGLVertexArray_tp_str(MGLVertexArray * self) {
	return PyUnicode_FromFormat("<ModernGL.VertexArray>");
}

PyObject * MGLVertexArray_render(MGLVertexArray * self, PyObject * args, PyObject * kwargs) {
	static const char * kwlist[] = {"mode", "vertices", "first", "instances", 0};

	MGLPrimitive * mode = MGL_TRIANGLES;
	int vertices = -1;
	int first = 0;
	int instances = 1;

	int args_ok = PyArg_ParseTupleAndKeywords(
		args,
		kwargs,
		"|O!I$II",
		(char **)kwlist,
		&MGLPrimitive_Type,
		&mode,
		&vertices,
		&first,
		&instances
	);

	if (!args_ok) {
		return 0;
	}

	if (vertices < 0) {
		if (self->num_vertices < 0) {
			MGLError * error = MGLError_New(TRACE, "Cannot detect the number of vertices");
			PyErr_SetObject((PyObject *)&MGLError_Type, (PyObject *)error);
			return 0;
		}

		vertices = self->num_vertices;
	}

	MGLPrimitive * gs_input = self->program->geometry_input;

	if (gs_input && gs_input->primitive != mode->geometry_primitive) {
		MGLError * error = MGLError_New(TRACE, "GeometryShader input is different from render mode");
		PyErr_SetObject((PyObject *)&MGLError_Type, (PyObject *)error);
		return 0;
	}

	const GLMethods & gl = self->context->gl;

	gl.UseProgram(self->program->obj);
	gl.BindVertexArray(self->obj);

	if (self->index_buffer != (MGLBuffer *)Py_None) {
		const void * ptr = (const void *)((GLintptr)first * 4);
		gl.DrawElementsInstanced(mode->primitive, vertices, GL_UNSIGNED_INT, ptr, instances);
	} else {
		gl.DrawArraysInstanced(mode->primitive, first, vertices, instances);
	}

	Py_RETURN_NONE;
}

const char * MGLVertexArray_render_doc = R"(
	render(mode, vertices, first, instances)

	The render primitive (mode) must be the same as the input primitive of the GeometryShader.

	Args:
		optional mode: By default :py:const:`~ModernGL.TRIANGLES` will be used.
		optional vertices: The number of vertices to transform.

	Keyword Args:
		first: The index of the first vertex to start with.
		instances: The number of instances.

	Returns:
		None
)";

PyObject * MGLVertexArray_transform(MGLVertexArray * self, PyObject * args, PyObject * kwargs) {
	static const char * kwlist[] = {"output", "mode", "vertices", "first", "instances", 0};

	MGLBuffer * output;
	MGLPrimitive * mode = MGL_TRIANGLES;
	int vertices = -1;
	int first = 0;
	int instances = 1;

	int args_ok = PyArg_ParseTupleAndKeywords(
		args,
		kwargs,
		"O!|O!I$IIO!",
		(char **)kwlist,
		&MGLBuffer_Type,
		&output,
		&MGLPrimitive_Type,
		&mode,
		&vertices,
		&first,
		&instances
	);

	if (!args_ok) {
		return 0;
	}

	if (vertices < 0) {
		if (self->num_vertices < 0) {
			MGLError * error = MGLError_New(TRACE, "Cannot detect the number of vertices");
			PyErr_SetObject((PyObject *)&MGLError_Type, (PyObject *)error);
			return 0;
		}

		vertices = self->num_vertices;
	}

	MGLPrimitive * gs_input = self->program->geometry_input;

	if (gs_input && gs_input->primitive != mode->geometry_primitive) {
		MGLError * error = MGLError_New(TRACE, "GeometryShader input is different from transform mode");
		PyErr_SetObject((PyObject *)&MGLError_Type, (PyObject *)error);
		return 0;
	}

	const GLMethods & gl = self->context->gl;

	gl.UseProgram(self->program->obj);
	gl.BindVertexArray(self->obj);

	gl.BindBufferBase(GL_TRANSFORM_FEEDBACK_BUFFER, 0, output->obj);

	gl.Enable(GL_RASTERIZER_DISCARD);
	gl.BeginTransformFeedback(mode->primitive);

	if (self->index_buffer != (MGLBuffer *)Py_None) {
		const void * ptr = (const void *)((GLintptr)first * 4);
		gl.DrawElementsInstanced(mode->primitive, vertices, GL_UNSIGNED_INT, ptr, instances);
	} else {
		gl.DrawArraysInstanced(mode->primitive, first, vertices, instances);
	}

	gl.EndTransformFeedback();
	gl.Disable(GL_RASTERIZER_DISCARD);
	gl.Flush();

	Py_RETURN_NONE;
}

const char * MGLVertexArray_transform_doc = R"(
	transform(output, mode, vertices, first, instances)

	Transform vertices.
	Stores the output in a single buffer.
	The transform primitive (mode) must be the same as the input primitive of the GeometryShader.

	Args:
		output: The buffer to store the output.
		optional mode: By default :py:const:`~ModernGL.TRIANGLES` will be used.
		optional vertices: The number of vertices to transform.

	Keyword Args:
		first: The index of the first vertex to start with.
		instances: The number of instances.

	Returns:
		None
)";

PyObject * MGLVertexArray_release(MGLVertexArray * self) {
	MGLVertexArray_Invalidate(self);
	Py_RETURN_NONE;
}

const char * MGLVertexArray_release_doc = R"(
	release()

	Release the vertex array.
)";

PyMethodDef MGLVertexArray_tp_methods[] = {
	{"render", (PyCFunction)MGLVertexArray_render, METH_VARARGS | METH_KEYWORDS, MGLVertexArray_render_doc},
	{"transform", (PyCFunction)MGLVertexArray_transform, METH_VARARGS | METH_KEYWORDS, MGLVertexArray_transform_doc},
	{"release", (PyCFunction)MGLVertexArray_release, METH_NOARGS, MGLVertexArray_release_doc},
	{0},
};

MGLProgram * MGLVertexArray_get_program(MGLVertexArray * self, void * closure) {
	Py_INCREF(self->program);
	return self->program;
}

char MGLVertexArray_program_doc[] = R"(
	program

	The program assinged to the VertexArray.
	The program will be used automatically when rendering and transforming.
)";

PyObject * MGLVertexArray_get_content(MGLVertexArray * self, void * closure) {
	Py_INCREF(self->content);
	return self->content;
}
char MGLVertexArray_content_doc[] = R"(
	content

	The content assinged to the VertexArray.
)";

PyObject * MGLVertexArray_get_index_buffer(MGLVertexArray * self, void * closure) {
	Py_INCREF(self->index_buffer);
	return (PyObject *)self->index_buffer;
}

char MGLVertexArray_index_buffer_doc[] = R"(
	index_buffer

	The index buffer if the index_buffer is set otherwise ``None``.
)";

PyObject * MGLVertexArray_get_vertices(MGLVertexArray * self, void * closure) {
	return PyLong_FromLong(self->num_vertices);
}

char MGLVertexArray_vertices_doc[] = R"(
	vertices

	The number of vertices detected.
	This is the minimum of the number of vertices possible per Buffer.
	The size of the index_buffer will be used when the VertexArray has a valid index_buffer set.
	Per instance vertex attributes does not affect this number.
)";

PyGetSetDef MGLVertexArray_tp_getseters[] = {
	{(char *)"program", (getter)MGLVertexArray_get_program, 0, MGLVertexArray_program_doc, 0},
	{(char *)"content", (getter)MGLVertexArray_get_content, 0, MGLVertexArray_content_doc, 0},
	{(char *)"index_buffer", (getter)MGLVertexArray_get_index_buffer, 0, MGLVertexArray_index_buffer_doc, 0},
	{(char *)"vertices", (getter)MGLVertexArray_get_vertices, 0, MGLVertexArray_vertices_doc, 0},
	{0},
};

const char * MGLVertexArray_tp_doc = R"(
	VertexArray
)";

PyTypeObject MGLVertexArray_Type = {
	PyVarObject_HEAD_INIT(0, 0)
	"ModernGL.VertexArray",                                 // tp_name
	sizeof(MGLVertexArray),                                 // tp_basicsize
	0,                                                      // tp_itemsize
	(destructor)MGLVertexArray_tp_dealloc,                  // tp_dealloc
	0,                                                      // tp_print
	0,                                                      // tp_getattr
	0,                                                      // tp_setattr
	0,                                                      // tp_reserved
	(reprfunc)MGLVertexArray_tp_str,                        // tp_repr
	0,                                                      // tp_as_number
	0,                                                      // tp_as_sequence
	0,                                                      // tp_as_mapping
	0,                                                      // tp_hash
	0,                                                      // tp_call
	(reprfunc)MGLVertexArray_tp_str,                        // tp_str
	0,                                                      // tp_getattro
	0,                                                      // tp_setattro
	0,                                                      // tp_as_buffer
	Py_TPFLAGS_DEFAULT | Py_TPFLAGS_BASETYPE,               // tp_flags
	MGLVertexArray_tp_doc,                                  // tp_doc
	0,                                                      // tp_traverse
	0,                                                      // tp_clear
	0,                                                      // tp_richcompare
	0,                                                      // tp_weaklistoffset
	0,                                                      // tp_iter
	0,                                                      // tp_iternext
	MGLVertexArray_tp_methods,                              // tp_methods
	0,                                                      // tp_members
	MGLVertexArray_tp_getseters,                            // tp_getset
	&MGLContextMember_Type,                                 // tp_base
	0,                                                      // tp_dict
	0,                                                      // tp_descr_get
	0,                                                      // tp_descr_set
	0,                                                      // tp_dictoffset
	(initproc)MGLVertexArray_tp_init,                       // tp_init
	0,                                                      // tp_alloc
	MGLVertexArray_tp_new,                                  // tp_new
};

MGLVertexArray * MGLVertexArray_New() {
	MGLVertexArray * self = (MGLVertexArray *)MGLVertexArray_tp_new(&MGLVertexArray_Type, 0, 0);
	return self;
}

void MGLVertexArray_Invalidate(MGLVertexArray * array) {
	if (Py_TYPE(array) == &MGLInvalidObject_Type) {

		#ifdef MGL_VERBOSE
		printf("MGLVertexArray_Invalidate %p already released\n", array);
		#endif

		return;
	}

	#ifdef MGL_VERBOSE
	printf("MGLVertexArray_Invalidate %p\n", array);
	#endif

	const GLMethods & gl = array->context->gl;
	gl.DeleteVertexArrays(1, (GLuint *)&array->obj);

	if (Py_REFCNT(array->program) == 2) {
		MGLProgram_Invalidate(array->program);
	}

	Py_DECREF(array->program);

	int content_len = PyTuple_GET_SIZE(array->content);

	for (int i = 0; i < content_len; ++i) {
		PyObject * tuple = PyTuple_GET_ITEM(array->content, i);
		MGLBuffer * buffer = (MGLBuffer *)PyTuple_GET_ITEM(tuple, 0);

		if (Py_REFCNT(buffer) == 2) {
			MGLBuffer_Invalidate(buffer);
		}
	}

	Py_DECREF(array->content);

	if (array->index_buffer != (MGLBuffer *)Py_None) {
		if (Py_REFCNT(array->index_buffer) == 2) {
			MGLBuffer_Invalidate(array->index_buffer);
		}
	}

	Py_DECREF(array->index_buffer);
	Py_DECREF(array->context);

	array->ob_base.ob_type = &MGLInvalidObject_Type;
	array->initial_type = &MGLVertexArray_Type;

	Py_DECREF(array);
}
