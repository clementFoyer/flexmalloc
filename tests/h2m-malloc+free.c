#include <stdio.h>
#include <stdlib.h>

int main (int argc, char *argv[])
{
	void *p_lc = NULL, *p_bw = NULL, *p_lt = NULL;
	printf ("Hello world\n");
	p_lc = malloc (16);
	if (!p_lc) { fprintf(stderr, "Error: unable to allocate memory (large cap)\n"); return 1; }
	free (p_lc);
	p_bw = malloc (16);
	if (!p_bw) { fprintf(stderr, "Error: unable to allocate memory (bandwidth)\n"); return 1; }
	free (p_bw);
	p_lt = malloc (16);
	if (!p_lt) { fprintf(stderr, "Error: unable to allocate memory (latency)\n"); return 1; }
	free (p_lt);
	printf ("Bye world\n");
	return 0;
}
