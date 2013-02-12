#include <stdio.h>
#include <stdlib.h>
#include <stdarg.h>
#include <string.h>
#include <errno.h>
#include <mach-o/loader.h>

void fatal(char *msg, ...);
void fatal(char *msg, ...) {
  va_list ap;

  va_start(ap, msg);
  vfprintf(stderr, msg, ap);
  fprintf(stderr, "\n");
  va_end(ap);

  exit(EXIT_FAILURE);
}

int main(int argc, char *argv[]) {
  if (argc != 3) {
      fatal("Syntax: %s <in_file> <out_file>", argv[0]);
  }

  char *inName = argv[1];
  char *outName = argv[2];

    FILE *inFile = fopen(inName, "r");
  if (inFile == NULL) {
      fatal("Unable to read %s: %s", inName, strerror(errno));
  }

  /* find out how big it is */
  fseek(inFile, 0, SEEK_END);
  long size = ftell(inFile);
  rewind(inFile);

  printf("%s: %lu bytes\n", inName, size);

  /* read it all into memory */
    unsigned char *buf = malloc(size);
  if (buf == NULL) {
      fatal("Out of memory");
  }
  if (fread(buf, 1, size, inFile) != size) {
      fatal("Error trying to read %s: %s", inName, strerror(errno));
  }
  if (fclose(inFile)) {
      fatal("Error trying to close %s: %s", inName, strerror(errno));
  }

  struct mach_header *mach_hdr = (struct mach_header *) (buf);
  printf("Mach magic: 0x%08x\n", mach_hdr->magic);
  if (mach_hdr->magic != MH_MAGIC) {
      fatal("Wrong magic number (expecting 0x%08x)", MH_MAGIC);
  }
  printf("CPU type: %d\n", ntohl(mach_hdr->cputype));
  printf("CPU sub-type: %d\n", ntohl(mach_hdr->cpusubtype));
  printf("*** Changing to CPU sub-type 11\n");
  mach_hdr->cpusubtype = 11;

  printf("Saving as %s\n", outName);

    FILE *outFile = fopen(outName, "w");
  if (outFile == NULL) {
      fatal("Unable to write %s: %s", outName, strerror(errno));
  }
  if (fwrite(buf, 1, size, outFile) != size) {
      fatal("Error trying to write %s: %s", outName, strerror(errno));
  }
  if (fclose(outFile)) {
      fatal("Error trying to close %s: %s", outName, strerror(errno));
  }

  return EXIT_SUCCESS;
}

