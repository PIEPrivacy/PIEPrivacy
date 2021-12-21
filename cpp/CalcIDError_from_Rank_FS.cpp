#define _USE_MATH_DEFINES
#include <iostream>
#include <cmath>
#include <map>
#include <tuple>
#include <stdio.h>
#include <string.h>
#include <stdlib.h>
//#include <math.h>
#include "mt19937ar2.h"

using namespace std;

#define Delta				1e-08
#define EARTH_RAD	6378.150	// Radius of the Earth [km]
#define DIS_PER_Y	(EARTH_RAD * 2 * M_PI) / 360	// Distance per one latitude [km]

FILE *FileOpen(const char *filename, const char *mode) {
	FILE *fp;

	if ((fp = fopen(filename, mode)) == NULL) {
		printf("cannot open %s\n", filename);
		exit(-1);
	}
	return fp;
}

// Randomly generate 0, 1, 2, ..., size-1, and store the first num values into rndperm
void MakeRndPerm(int *rndperm, int size, int num) {
	int rnd;
	int *ordperm;
	int i, j;

	// 0, 1, 2, ..., size-1 --> ordperm
	ordperm = (int *)malloc(size * sizeof(int));
	for (i = 0; i < size; i++) {
		ordperm[i] = i;
	}

	for (i = 0; i < num; i++) {
		rnd = genrand_int32() % (size - i);
		rndperm[i] = ordperm[rnd];
		for (j = rnd + 1; j < size - i; j++) {
			ordperm[j - 1] = ordperm[j];
		}
	}

	free(ordperm);
}

int main(int argc, char *argv[])
{
	char outdir[129], city[129], obf[129];
	int adv_know, reid_method, req_num_loc, num_loc, st_user, en_user, user_num;
	char rankfile[129], outfile[129];
	int vuser, rank, same_score_num;
	map<int, int> vuser_rank;
	map<int, int>::iterator vitr;
	int *rndperm;
	int iderror, gescorenum;
	int i, j;
	char s[1025];
	FILE *fp;
	char *tok;

	// Initialization of Mersennne Twister
	unsigned long init[4] = { 0x123, 0x234, 0x345, 0x456 }, length = 4;
	init_by_array(init, length);

	if (argc < 4) {
		printf("Usage: %s [DataDir] [City] [Obf] ([AdvKnow (default:2)] [ReidMethod (default:1)] [ReqNumLoc (default:1)] [NumLoc (default:0)] [StUser (default:0)] [EnUser (default:0)] [#Users (default:0)])\n\n", argv[0]);
		printf("[DataDir]: Data directory\n");
		printf("[City]: City (AL/IS/JK/KL/NY/SP/TK)\n");
		printf("[Obf]: Obfuscation (org/RR-e[Epsilon]-l[TraceLen]/GLH-e[Epsilon]-m1-g[g]-l[TraceLen]/NS-nt[NoiseType]-np[NoiseParam]-l[TraceLen])\n");
		printf("[AdvKnow]: Adversary's knowledge (1: partial knowledge, 2: maximum knowledge (training = original), 3: maximum knowledge (training = obfuscated))\n");
		printf("[ReidMethod]: Re-identification method (1:transition matrix, 2:visiting probability vector, >=3:visiting probability vector with Gaussian kernel (std: 10^([ReidMethod]-3) [m]))\n");
		printf("[ReqNumLoc]: Required number of training locations per user (used for pruning users)\n");
		printf("[NumLoc]: Number of training locations per user (0: all)\n");
		printf("[StUser]: Start user index\n");
		printf("[EnUser]: End user index (0: last user index)\n");
		printf("[#Users]: Number of users (0: all users)\n\n");
		return -1;
	}

	sprintf(outdir, argv[1]);
	sprintf(city, argv[2]);
	sprintf(obf, argv[3]);

	adv_know = 2;
	if (argc >= 5) adv_know = atoi(argv[4]);
	reid_method = 1;
	if (argc >= 6) reid_method = atoi(argv[5]);
	req_num_loc = 1;
	if (argc >= 7) req_num_loc = atoi(argv[6]);
	num_loc = 0;
	if (argc >= 8) num_loc = atoi(argv[7]);
	st_user = 0;
	if (argc >= 9) st_user = atoi(argv[8]);
	en_user = 0;
	if (argc >= 10) en_user = atoi(argv[9]);
	user_num = 0;
	if (argc >= 11) user_num = atoi(argv[10]);
//	printf("%s,%s,%d,%d,%d,%d\n", outdir, city, adv_know, reid_method, req_num_loc, num_loc);

	// Read genuine scores and ranks
	sprintf(rankfile, "%s/ge_score-rank_%s_ak%d_rm%d_rnl%d_nl%d_u%d-%d.csv", outdir, city, adv_know, reid_method, req_num_loc, num_loc, st_user, en_user);
	if (strcmp(obf, "org") != 0) sprintf(rankfile, "%s/ge_score-rank_%s_%s_ak%d_rm%d_rnl%d_nl%d_u%d-%d.csv", outdir, city, obf, adv_know, reid_method, req_num_loc, num_loc, st_user, en_user);

	fp = FileOpen(rankfile, "r");
	// Skip the header
	fgets(s, 1024, fp);
	iderror = 0;
	gescorenum = 0;
	while (fgets(s, 1024, fp) != NULL) {
		tok = strtok(s, ",");
		vuser = atoi(tok);
		tok = strtok(NULL, ",");
		tok = strtok(NULL, ",");
		tok = strtok(NULL, ",");
		rank = atoi(tok);
		tok = strtok(NULL, ",");
		same_score_num = atoi(tok);
//		printf("%d,%d,%d,", vuser, rank, same_score_num);

		// If there are multiple users who have the same score, randomly decide the rank --> rank
		if (same_score_num > 0){
			rndperm = (int *)malloc(sizeof(int)* (same_score_num+1));
			MakeRndPerm(rndperm, same_score_num+1, 1);
			rank += rndperm[0];
//			printf("%d,%d", rndperm[0], rank);
			free(rndperm);
		}
//		printf("\n");

		// Rank --> vuser_rank
		vuser_rank[vuser] = rank;

		// If rank > 1, increase iderror
		if (rank > 1) iderror++;
		gescorenum++;
		if (gescorenum == user_num) break;
	}
	fclose(fp);

	// Output iderror
	sprintf(outfile, "%s/iderror_%s_ak%d_rm%d_rnl%d_nl%d_u%d-%d-%d.csv", outdir, city, adv_know, reid_method, req_num_loc, num_loc, st_user, en_user, user_num);
	if (strcmp(obf, "org") != 0) sprintf(outfile, "%s/iderror_%s_%s_ak%d_rm%d_rnl%d_nl%d_u%d-%d-%d.csv", outdir, city, obf, adv_know, reid_method, req_num_loc, num_loc, st_user, en_user, user_num);

	fp = FileOpen(outfile, "w");
	fprintf(fp, "#errors,#gescores,error_rate\n");
	printf("#errors,#gescores,error rate\n");
	fprintf(fp, "%d,%d,%f\n", iderror, gescorenum, (double)iderror/(double)gescorenum);
	printf("%d,%d,%f\n", iderror, gescorenum, (double)iderror/(double)gescorenum);
	fprintf(fp, "vuser,rank\n");
	printf("vuser,rank\n");
	for (vitr = vuser_rank.begin(); vitr != vuser_rank.end(); vitr++) {
		vuser = vitr->first;
		fprintf(fp, "%d,%d\n", vuser, vuser_rank[vuser]);
		printf("%d,%d\n", vuser, vuser_rank[vuser]);
	}
	fclose(fp);

	return 0;
}
