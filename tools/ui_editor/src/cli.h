#ifndef UIED_CLI_H
#define UIED_CLI_H
// Returns -1 if argv isn't a CLI mode (then the window opens), else an exit code.
int cli_run(const char *root, int argc, char **argv);
#endif
