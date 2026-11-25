# Conversion test cases

Each case is a text file which `tests/run-tests.sh` converts to binary, back
to text, and to binary again. The case passes when:

* the first conversion returns the expected exit status (0 by default);
* nothing is written to stderr, unless the case expects a message;
* the decoded text converts back to an identical binary file;
* no AddressSanitizer error is reported (when run against a sanitizer build).

The input for a case is either:

* `<name>.txt` - the text to convert; or
* `<name>.gen` - a Bash script which writes the text to stdout, for inputs
  too large to keep in the repository.

An optional `<name>.expect` file changes what is expected. Blank lines and
lines starting with `#` are ignored. Each other line is a keyword and a value:

| Keyword | Meaning |
| --- | --- |
| `encode-rc <n>` | The first conversion must exit with status `<n>`. When it is not 0, the conversion back is not attempted. |
| `stderr-contains <text>` | stderr from the first conversion must contain `<text>`. Other messages are then allowed. |
