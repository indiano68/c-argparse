#include <c-argparse.h>
#include <stddef.h>
#include <stdio.h>

char *carg_error_to_string(carg_error_t error_level) {
  switch (error_level) {
  case CARG_SUCCESS:
    return "CARG_SUCCESS";
  case CARG_INSUFFICIENT_ARGS:
    return "CARG_INSUFFICIENT_ARGS";
  case CARG_UNKNOWN_ARG:
    return "CARG_UNKNOWN_ARG";
  case CARG_UNKNOWN_FAILURE:
    return "CARG_UNKNOWN_FAILURE";
  case CARG_EXCESSIVE_POSITIONAL:
    return "CARG_EXCESSIVE_POSITIONAL";
  }
}
int main(int argc, char **argv) {
  size_t size              = 0;
  size_t maxit             = 0;
  size_t m_size            = 0;
  double tol               = 0;
  double rtol              = 0;
  char name[]              = "";
  carg_error_t error_level = CARG_SUCCESS;

  int num_positionals;
  int num_options;

  carg_pos_t positionals[] = {
      {CARG_STRING, name}, {CARG_FLOAT, &rtol}, {CARG_UNSIGNED_LONG, &m_size}};
  carg_opt_t options[] = {{"size", 's', CARG_UNSIGNED_LONG, &size},
                          {"tollerance", 't', CARG_FLOAT, &tol},
                          {"max_it", 0, CARG_UNSIGNED_LONG, &maxit}};
  num_positionals      = sizeof(positionals) / sizeof(carg_pos_t);
  num_options          = sizeof(options) / sizeof(carg_opt_t);

  carg_print_args(positionals, num_positionals, options, num_options);
  error_level = carg_parse_args(argc, argv, positionals, num_positionals,
                                options, num_options);
  printf("%s\n", carg_error_to_string(error_level));
  carg_print_args(positionals, num_positionals, options, num_options);
}
