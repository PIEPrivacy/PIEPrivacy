#include <iostream>
#include <map>
#include <tuple>
#include <random>
#include <fstream>
#include <stdio.h>
#include <string.h>
#include <stdlib.h>
#include <assert.h>
#include <math.h>
#include "mt19937ar2.h"

using namespace std;

struct ScoreVecType {
	int euser_id;
	double score;
};

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

int main(int argc, char *argv[])
{
	char inoutdir[129], filename[129];
	int rnd_imscore, sim;
	char gescorefile[1025], imscorefile[1025], outfile[1025];
	char s[1025];
	int totgenum, totimnum;
	map<int, double> gescore;
	map<tuple<int, int>, double> imscore;
	map<int, double>::iterator gitr;
	map<tuple<int, int>, double>::iterator iitr;
//	double **score_mat;
	int vuser, euser;
	double score;
//	struct ScoreVecType *scorevec;
//	int *rank;
	map<int, int> rank;
	map<int, int> tie;
	map<int, int>::iterator ritr;
	int rnd;
	int id_error;
	char *tok;
	int i, j;
	FILE *fp;

	// Initialization of Mersennne Twister
	unsigned long init[4] = { 0x123, 0x234, 0x345, 0x456 }, length = 4;
	init_by_array(init, length);

	if (argc < 3) {
		printf("Usage: %s [DataDir] [FileName] ([RndImscore (default:1)] [Sim (default:1)])\n\n", argv[0]);
		printf("[DataDir]: Data directory\n");
		printf("[FileName]: File name (right before the suffix)\n");
		printf("[RndImscore]: Number of randomly selected impostor scores per user (0: all)\n");
		printf("[Sim]: Similarity or distance (1: similarity, 0: distance)\n\n");

		return -1;
	}

	sprintf(inoutdir, argv[1]);
	sprintf(filename, argv[2]);

	rnd_imscore = 1;
	if (argc >= 4) rnd_imscore = atoi(argv[3]);
	printf("%s,%s,%d\n", inoutdir, filename, rnd_imscore);
	sim = 1;
	if (argc >= 5) sim = atoi(argv[4]);

	// gescorefile, imscorefile, outfile
	sprintf(gescorefile, "%s/ge_score_%s.csv", inoutdir, filename);
	sprintf(imscorefile, "%s/im_score_%s_r%d.csv", inoutdir, filename, rnd_imscore);
	sprintf(outfile, "%s/iderror_%s_r%d.csv", inoutdir, filename, rnd_imscore);

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

	/*
	// Malloc
	score_mat = (double **)malloc(sizeof(double *) * totgenum);
	for(i = 0; i < totgenum; i++) score_mat[i] = (double *)malloc(sizeof(double) * totgenum);
	rank = (int *)malloc(sizeof(int) * totgenum);
	scorevec = (struct ScoreVecType *)malloc(sizeof(struct ScoreVecType) * totgenum);
	*/

	// Read genuine scores --> gescore
	printf("Reading genuine scores... ");
	fp = FileOpen(gescorefile, "r");
	fgets(s, 1024, fp);
	for (i = 0; i < totgenum; i++) {
		fgets(s, 1024, fp);
		tok = strtok(s, ",");
		vuser = atoi(tok);
		tok = strtok(NULL, ",");
		tok = strtok(NULL, ",");
		score = atof(tok);
		if (sim) gescore[vuser] = score;
		else gescore[vuser] = -score;
		/*
		if(vuser >= totgenum){
			printf("Wrong format @ genuine score file.\n");
			exit(-1);
		}
		score_mat[vuser][vuser] = score;
		*/
	}
	fclose(fp);
	printf("done.\n");
	printf("#gescores: %d\n", totgenum);
	/*
	for (gitr = gescore.begin(); gitr != gescore.end(); gitr++) {
		cout << gitr->first << " " << gitr->second << endl;
	}
	*/

	// Read impostor scores --> imuserscore
	printf("Reading impostor scores... ");
	fp = FileOpen(imscorefile, "r");
	fgets(s, 1024, fp);
	for (i = 0; i < totimnum; i++) {
		fgets(s, 1024, fp);
		tok = strtok(s, ",");
		vuser = atoi(tok);
		tok = strtok(NULL, ",");
		euser = atoi(tok);
		tok = strtok(NULL, ",");
		score = atof(tok);
		if (sim) imscore[make_tuple(vuser, euser)] = score;
		else imscore[make_tuple(vuser, euser)] = -score;
		/*
		if(vuser >= totgenum || euser >= totgenum){
			printf("Wrong format @ impostor score file.\n");
			exit(-1);
		}
		score_mat[vuser][euser] = score;
		*/
	}
	fclose(fp);
	printf("done.\n");
	printf("#imscores: %d\n", totimnum);
	/*
	for (iitr = imscore.begin(); iitr != imscore.end(); iitr++) {
		cout << get<0>(iitr->first) << " " << get<1>(iitr->first) << " " << iitr->second << endl;
	}
	*/

	// Calculate ranks and the number of ties --> rank, tie
	for (gitr = gescore.begin(); gitr != gescore.end(); gitr++) {
		vuser = gitr->first;
		rank[vuser] = 1;
		tie[vuser] = 1;
	}
	for (iitr = imscore.begin(); iitr != imscore.end(); iitr++) {
		vuser = get<0>(iitr->first);
		euser = get<1>(iitr->first);
		score = iitr->second;
		if(score > gescore[vuser]) rank[vuser] += 1;
		else if(score == gescore[vuser]) tie[vuser] += 1;
	}

	// Tie break
	for (gitr = gescore.begin(); gitr != gescore.end(); gitr++) {
		vuser = gitr->first;
		if(tie[vuser] > 1){
			rnd = genrand_int32() % tie[vuser];
			rank[vuser] += rnd;
//			cout << vuser << " " << tie[vuser] << " " << rnd << " " << rank[vuser] << endl;
		}
	}

	// Identification errors --> id_error
	id_error = 0;
	for (gitr = gescore.begin(); gitr != gescore.end(); gitr++) {
		vuser = gitr->first;
		if(rank[vuser] > 1) id_error++;
	}

	// Output the results
	fp = FileOpen(outfile, "w");
	fprintf(fp, "#errors,#gescores,error_rate\n");
	printf("#errors,#gescores,error rate\n");
	fprintf(fp, "%d,%d,%f\n", id_error, totgenum, (double)id_error/(double)totgenum);
	printf("%d,%d,%f\n", id_error, totgenum, (double)id_error/(double)totgenum);
	fprintf(fp, "vuser,rank\n");
	printf("vuser,rank\n");
	for (gitr = gescore.begin(); gitr != gescore.end(); gitr++) {
		vuser = gitr->first;
		fprintf(fp, "%d,%d\n", vuser, rank[vuser]);
		printf("%d,%d\n", vuser, rank[vuser]);
	}
	fclose(fp);

	return 0;
}

