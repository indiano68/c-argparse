#include <c-argparse.h>
#include <stddef.h>
#include <stdio.h>

int main(int argc, char **argv) {
  int size                 = 0;
  size_t maxit             = 0;
  size_t m_size            = 0;
  double tol               = 0;
  double rtol              = 0;
  int show_config          = 0;
  char *name               = NULL;
  carg_error_t error_level = CARG_SUCCESS;

  int num_positionals;
  int num_options;

  carg_pos_t positionals[] = {{CARG_STRING, (void *)&name},
                              {CARG_DOUBLE, &rtol},
                              {CARG_UINT64, &m_size}};
  carg_opt_t options[]     = {{"size", 's', CARG_INT32, &size},
                              {"tollerance", 't', CARG_DOUBLE, &tol},
                              {"show_config", 'p', CARG_BOOL, &show_config},
                              {"max_it", 0, CARG_UINT64, &maxit}};
  num_positionals          = sizeof(positionals) / sizeof(carg_pos_t);
  num_options              = sizeof(options) / sizeof(carg_opt_t);

  // carg_print_args(positionals, num_positionals, options, num_options);
  error_level = carg_parse_args(argc, argv, positionals, num_positionals,
                                options, num_options);
  printf("%s\n", carg_strerror(error_level));
  carg_print_args(positionals, num_positionals, options, num_options);
}
