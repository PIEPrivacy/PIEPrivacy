#include <random>
#include <fstream>
#include <stdio.h>
#include <string.h>
#include <stdlib.h>
#include <assert.h>
#include <math.h>
#include "mt19937ar2.h"

FILE *FileOpen(const char *filename, const char *mode) {
	FILE *fp;

	if ((fp = fopen(filename, mode)) == NULL) {
		printf("cannot open %s\n", filename);
		exit(-1);
	}
	return fp;
}

int compare_double(double *x, double *y) {
	if (*x > *y)       return(1);   /* return positive integer */
	else if (*x == *y) return(0);   /* return zero     integer */
	else              return(-1);  /* return negative integer */
}

// Calculate FAR and FRR
void CalcFARFRR(char *outfile, double *gescore, int totgenum, double *imscore, int totimnum, int thr_num){
	double max_score, min_score, thr_shift;
	double thr;
	int fa, fr;
	int gfreq, ifreq;
	int gp, ip;
	FILE *fp;

	// Initialization
	max_score = std::max(gescore[totgenum - 1], imscore[totimnum - 1]);
	min_score = std::min(gescore[0], imscore[0]);
	thr_shift = (max_score - min_score) / (double)thr_num;

	// Open outfile
	fp = FileOpen(outfile, "w");

	fprintf(fp, "thr,#imscores,imfreq,#gescores,gefreq,#FA,#FR,FAR,FRR\n");

	// Calculate FAR and FRR while changing a threshold
	fa = totimnum;
	fr = 0;
	gp = ip = 0;

	for (thr = min_score; thr <= max_score; thr += thr_shift){
		gfreq = ifreq = 0;
		// If there exists a genuine score below the threshold, increase gfreq and fr
		while (gp < totgenum && gescore[gp] < thr){
			gfreq++;
			fr++;
			gp++;
		}
		// If there exists an impostor score below the threshold, increase ifreq and decrease fa
		while (ip < totimnum && imscore[ip] < thr){
			ifreq++;
			fa--;
			ip++;
		}
		// Output the results
		fprintf(fp, "%e,%d,%e,%d,%e,%d,%d,%e,%e\n", thr, ifreq, ifreq / (double)totimnum, gfreq, gfreq / (double)totgenum, fa, fr, (double)fa / (double)totimnum, (double)fr / (double)totgenum);
	}

	fclose(fp);
}

int main(int argc, char *argv[])
{
	char inoutdir[129], filename[129], outfile[129];
	int rnd_imscore, thr_num, sim;
	char gescorefile[1025], imscorefile[1025];
	int un;
	char s[1025];
	int totgenum, totimnum;
	double *gescore, *imscore;
	int *rank;
	int genum, imnum;
	int eval_vuser;
	char *tok;
	int i, j, x;
	FILE *fp;

	// Initialization of Mersennne Twister
	unsigned long init[4] = { 0x123, 0x234, 0x345, 0x456 }, length = 4;
	init_by_array(init, length);

	if (argc < 3) {
		printf("Usage: %s [DataDir] [FileName] ([RndImscore (default:1)] [ThrNum (default:1000)] [Sim (default:1)])\n\n", argv[0]);
		printf("[DataDir]: Data directory\n");
		printf("[FileName]: File name (right before the suffix)\n");
		printf("[RndImscore]: Number of randomly selected impostor scores per user (0: all)\n");
		printf("[ThrNum]: Number of thresholds\n");
		printf("[Sim]: Similarity or distance (1: similarity, 0: distance)\n");

		return -1;
	}

	sprintf(inoutdir, argv[1]);
	sprintf(filename, argv[2]);

	rnd_imscore = 1;
	if (argc >= 4) rnd_imscore = atoi(argv[3]);
	thr_num = 1000;
	if (argc >= 5) thr_num = atoi(argv[4]);
	printf("%s,%s,%d,%d\n", inoutdir, filename, rnd_imscore, thr_num);
	sim = 1;
	if (argc >= 6) sim = atoi(argv[5]);
	printf("%s,%s,%d,%d,%d\n", inoutdir, filename, rnd_imscore, thr_num, sim);

	// gescorefile, imscorefile, outfile
	sprintf(gescorefile, "%s/ge_score_%s.csv", inoutdir, filename);
	sprintf(imscorefile, "%s/im_score_%s_r%d.csv", inoutdir, filename, rnd_imscore);
	sprintf(outfile, "%s/FARFRR_%s_r%d.csv", inoutdir, filename, rnd_imscore);

	// Read the total number of genuine scores from gescorefile --> totgenum
	fp = FileOpen(gescorefile, "r");
	fgets(s, 1024, fp);
	totgenum = 0;
	while (fgets(s, 1024, fp) != NULL) totgenum++;
	fclose(fp);

	// Read the total number of impostor scores from imscorefile --> totimnum
	fp = FileOpen(imscorefile, "r");
	fgets(s, 1024, fp);
	totimnum = 0;
	while (fgets(s, 1024, fp) != NULL) totimnum++;
	fclose(fp);

	// Malloc
	gescore = (double *)malloc(sizeof(double) * totgenum);
	imscore = (double *)malloc(sizeof(double) * totimnum);
	rank = (int *)malloc(sizeof(int) * totgenum);

	// Read genuine scores and ranks --> gescore, rank
	printf("Reading genuine scores... ");
	fp = FileOpen(gescorefile, "r");
	fgets(s, 1024, fp);
	for (i = 0; i < totgenum; i++) {
		fgets(s, 1024, fp);
		tok = strtok(s, ",");
		tok = strtok(NULL, ",");
		tok = strtok(NULL, ",");
		gescore[i] = atof(tok);
		if(sim == 0) gescore[i] = - gescore[i];
		tok = strtok(NULL, ",");
		tok[strlen(tok) - 1] = '\0';
		if(tok[0] != '-') rank[i] = atoi(tok);
//		printf("%d,%f,%d\n", i, gescore[i],rank[i]);
	}
	fclose(fp);
	printf("done.\n");
	printf("#gescores: %d\n", totgenum);

	// Read impostor scores --> imscore
	printf("Reading impostor scores... ");
	fp = FileOpen(imscorefile, "r");
	fgets(s, 1024, fp);
	for (i = 0; i < totimnum; i++) {
		fgets(s, 1024, fp);
		tok = strtok(s, ",");
		tok = strtok(NULL, ",");
		tok = strtok(NULL, ",");
		imscore[i] = atof(tok);
		if(sim == 0) imscore[i] = - imscore[i];
//		printf("%d,%f\n", i, imscore[i]);
	}
	fclose(fp);
	printf("done.\n");
	printf("#imscores: %d\n", totimnum);

	// Sort gescore and imscore
	qsort(gescore, totgenum, sizeof(double), (int(*) (const void *, const void *))compare_double);
	qsort(imscore, totimnum, sizeof(double), (int(*) (const void *, const void *))compare_double);

	CalcFARFRR(outfile, gescore, totgenum, imscore, totimnum, thr_num);

	// free
	free(gescore);
	free(imscore);
	free(rank);

	return 0;
}

