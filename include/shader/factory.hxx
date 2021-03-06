#include <streambuf>
#include <iostream>
#include <string.h>
#include <fstream>
#include <string>
#include <err.h>
#include <epoxy/gl.h>

#include "shader/factory.hh"

using namespace std;

template <typename TList>
ShaderFactory<TList>::ShaderFactory(const char *shader_code,
                                    unsigned shader_type):
  shader(strdup(shader_code)), shader_type(shader_type)
{
  structures.push_back(new StructureFactory(NULL));
  structures.push_back(new StructureFactory(NULL));
}

template <typename TList>
ShaderFactory<TList>::~ShaderFactory() {
  free(shader);
  for (size_t i = 0; i < structures.size(); ++i)
    delete structures[i];

  for (size_t i = 0; i < inputs.size(); ++i)
    free(inputs[i]);
  for (size_t i = 0; i < uniforms.size(); ++i)
    free(uniforms[i]);
  for (size_t i = 0; i < outputs.size(); ++i)
    free(outputs[i]);
}


template <typename TList>
template <typename T>
ShaderFactory<T> ShaderFactory<TList>::from_file(const char *file,
                                                 unsigned shader_type) {

  std::ifstream t(file);
  std::string str((std::istreambuf_iterator<char>(t)),
                   std::istreambuf_iterator<char>());

  return ShaderFactory<T>(str.c_str(), shader_type);
}

template <typename TList>
char *ShaderFactory<TList>::make_variable(const char *type, const char *name) {

  int len1 = strlen(type);
  int len2 = strlen(name);

  /*
   * We need to allocate the space for two more
   * chars:
   *   * the space between the type and the name
   *   * the semicolon at the end of the instruction
   * member = "type name;\0"
   */
  char *var = (char *) malloc(len1 + len2 + 3);

  strcpy(var, type);
  strcpy(var + len1 + 1, name);

  var[len1] = ' ';
  var[len1 + len2 + 1] = ';';
  var[len1 + len2 + 2] = '\0';

  return var;
}

static const char *version = "#version 330\n";
static const char *main_begin = "void main () {";
static const char *main_end = "}";

static const char *input_keyword = "in ";
static const char *uniform_keyword = "uniform ";
static const char *output_keyword = "out ";

static size_t version_len = sizeof ("#version 330\n") - 1;
static size_t main_begin_len = sizeof ("void main () {") - 1;
static size_t main_end_len = sizeof ("}") - 1;

static size_t input_keyword_len = sizeof ("in ") - 1;
static size_t uniform_keyword_len = sizeof ("uniform ") - 1;
static size_t output_keyword_len = sizeof ("out ") - 1;

template <typename TList>
char *ShaderFactory<TList>::compose() {
  size_t len1 = strlen(shader);
  size_t structs_len = 0, inputs_len = 0, uniforms_len = 0,
         outputs_len = 0, instructions_len = 0;

  for (size_t i = 0; i < structures.size(); ++i)
    structs_len += structures[i]->len;

  for (size_t i = 0; i < inputs.size(); ++i)
    inputs_len += strlen(inputs[i]) + input_keyword_len;
  for (size_t i = 0; i < uniforms.size(); ++i)
    uniforms_len += strlen(uniforms[i]) + uniform_keyword_len;
  for (size_t i = 0; i < outputs.size(); ++i)
    outputs_len += strlen(outputs[i]) + output_keyword_len;
  for (size_t i = 0; i < instructions.size(); ++i)
    instructions_len += strlen(instructions[i]);

  char *code = (char *) malloc (len1 + structs_len +
                                inputs_len + uniforms_len +
                                outputs_len + instructions_len +
                                version_len + main_begin_len +
                                main_end_len + version_len);
  const char *begin = shader;

  // will crash if there is no \n
  if (!strncmp(shader, "#version", sizeof ("#version") - 1)) {
    begin = strchr(code, '\n');
    shader += (size_t) (begin - code);
  }

  size_t s = sizeof ("#version 330\n") - 1;
  strcpy(code, "#version 330\n");

  for (size_t i = 0; i < structures.size(); ++i) {
    size_t len = 0;
    char *struc = structures[i]->generate(&len);
    if (struc)
      strcpy(code + s, struc);
    free(struc);
    s += len;
  }

  for (size_t i = 0; i < inputs.size(); ++i) {
    strcpy(code + s, input_keyword);
    s += input_keyword_len;
    strcpy(code + s, inputs[i]);
    s += strlen(inputs[i]);
  }

  for (size_t i = 0; i < uniforms.size(); ++i) {
    strcpy(code + s, uniform_keyword);
    s += uniform_keyword_len;
    strcpy(code + s, uniforms[i]);
    s += strlen(uniforms[i]);
  }

  for (size_t i = 0; i < outputs.size(); ++i) {
    strcpy(code + s, output_keyword);
    s += output_keyword_len;
    strcpy(code + s, outputs[i]);
    s += strlen(outputs[i]);
  }

  for (size_t i = 0; begin != shader + len1 &&
       strncmp(begin, "void main", sizeof ("void main" ) -1);
       ++begin, ++i, s++)
    (code + s)[i] = *begin;

  strcpy(code + s, main_begin);
  s += main_begin_len;

  for (size_t i = 0; i < instructions.size(); ++i) {
    strcpy(code + s, instructions[i]);
    s += strlen(instructions[i]);
  }

  strcpy(code + s, main_end);

  return code;
}
