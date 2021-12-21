#include <random>
#include <fstream>
#include <stdio.h>
#include <string.h>
#include <stdlib.h>
#include <assert.h>
#include <math.h>
#include "mt19937ar2.h"

struct UserScoreType {
	int vuser;
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

// Randomly generate 0, 1, 2, ..., size-1, and store them into rndperm
void MakeRndPerm(int *rndperm, int size) {
	int rnd;
	int *ordperm;
	int i, j;

	// 0, 1, 2, ..., size-1 --> ordperm
	ordperm = (int *)malloc(size * sizeof(int));
	for (i = 0; i < size; i++) {
		ordperm[i] = i;
	}

	for (i = 0; i < size; i++) {
		rnd = genrand_int32() % (size - i);
		rndperm[i] = ordperm[rnd];
		for (j = rnd + 1; j < size - i; j++) {
			ordperm[j - 1] = ordperm[j];
		}
	}

	free(ordperm);
}

// Compute the digamma function (from http://web.science.mq.edu.au/~mjohnson/code/digamma.c)
double digamma(double x) {
	double result = 0, xx, xx2, xx4;
	assert(x > 0);
	for (; x < 7; ++x)
		result -= 1 / x;
	x -= 1.0 / 2.0;
	xx = 1.0 / x;
	xx2 = xx * xx;
	xx4 = xx2 * xx2;
	result += log(x) + (1. / 24.)*xx2 - (7.0 / 960.0)*xx4 + (31.0 / 8064.0)*xx4*xx2 - (127.0 / 30720.0)*xx4*xx4;
	return result;
}

// Calculate the KL divergence based on the generalized kNN estimator [Wang+, T-IT09]
double KL_GenNN(double *gescore, double *imscore, int genum, int imnum) {
	double ns;
	double kl = 0.0;
	double epsilon, rho, nu;
	double rho_li, nu_ki;
	int li, ki;
	double term1, term2, term3;

	double dis;
	int i, j;

	double sigma = 0.0001;

	// Initialization
	std::random_device seed_gen;
	std::default_random_engine engine(seed_gen());
	std::normal_distribution<> dist(0.0, sigma);

	// Add noise from the Gaussian distribution N(0,sigma^2) --> gescore, imscore
	for (i = 0; i < genum; i++) {
		ns = dist(engine);
		gescore[i] += ns;
		//		printf("%d,%f,%f\n", i, gescore[i], ns);
	}
	for (i = 0; i < imnum; i++) {
		ns = dist(engine);
		imscore[i] += ns;
		//		printf("%d, %f,%f\n", i, imscore[i], ns);
	}

	// For each gescore
	term1 = term2 = term3 = 0.0;
	for (i = 0; i < genum; i++) {
		// Minimum distance between the i-th gescore and the other gescores --> rho
		rho = -1.0;
		for (j = 0; j < genum; j++) {
			if (i == j) continue;
			dis = fabs(gescore[i] - gescore[j]);
			if (rho == -1.0 || rho > dis) rho = dis;

			//			fp = FileOpen("debug_gescore.csv", "a");
			//			fprintf(fp, "%d,%f,%f,%f,%f\n", j, gescore[i], gescore[j], dis, rho);
			//			fclose(fp);
		}
		// Minimum distance between the i-th gescore and imscores --> nu
		nu = -1.0;
		for (j = 0; j < imnum; j++) {
			dis = fabs(gescore[i] - imscore[j]);
			if (nu == -1.0 || nu > dis) nu = dis;

			//			fp = FileOpen("debug_imscore.csv", "a");
			//			fprintf(fp, "%d,%f,%f,%f,%f\n", j, gescore[i], imscore[j], dis, nu);
			//			fclose(fp);
		}
		// max(rho, nu) --> epsilon
		epsilon = std::max(rho, nu);

		// Number of samples whose distances to the i-th gescore are less than or equal to epsilon --> li
		// Maximum distance to the i-th gescore among the samples --> rho_li
		li = 0;
		rho_li = -1.0;
		for (j = 0; j < genum; j++) {
			if (i == j) continue;
			dis = fabs(gescore[i] - gescore[j]);
			if (dis <= epsilon) {
				li++;
				if (rho_li < dis) rho_li = dis;
			}
		}

		// Number of samples whose distances to the i-th gescore are less than or equal to epsilon --> ki
		// Maximum distance to the i-th gescore among the samples --> nu_ki
		ki = 0;
		nu_ki = -1.0;
		for (j = 0; j < imnum; j++) {
			dis = fabs(gescore[i] - imscore[j]);
			if (dis <= epsilon) {
				ki++;
				if (nu_ki < dis) nu_ki = dis;
			}
		}

		// Update the 1st term
		if (nu_ki != 0.0 || rho_li != 0.0) term1 += log(nu_ki / rho_li);
		// Update the 2nd term
		term2 += digamma((double)li) - digamma((double)ki);

		//		printf("%d,%f,%f,%f,%d,%f,%d,%f,%f,%f\n", i, rho, nu, epsilon, li, rho_li, ki, nu_ki, term1, term2);
	}
	// Compute the 1st term
	term1 /= (double)genum;
	// Compute the 2nd term
	term2 /= (double)genum;
	// Compute the 3rd term
	term3 = log((double)imnum / ((double)genum - 1.0));
	// Compute the KL divergence (nat)
	kl = term1 + term2 + term3;

	// Transform nat into bit
	kl = kl / log(2.0);

	//	printf("%f,%f,%f\n", term1, term2, term3);
	//	printf("%f\n", kl);

	return kl;
}

// Calculate the KL divergence based on the kNN estimator [Wang+, T-IT09]
double KL_kNN(double *gescore, double *imscore, int genum, int imnum, int k) {
	double ns;
	double kl = 0.0;
	double rho, nu;
	double term1, term3;

	double *ge_dis, *im_dis;
	int i, j, x;
	FILE *fp;

	double sigma = 0.0001;

	// Initialization
	std::random_device seed_gen;
	std::default_random_engine engine(seed_gen());
	std::normal_distribution<> dist(0.0, sigma);

	// Add noise from the Gaussian distribution N(0,sigma^2) --> gescore, imscore
	for (i = 0; i < genum; i++) {
		ns = dist(engine);
		gescore[i] += ns;
		//		printf("%d,%f,%f\n", i, gescore[i], ns);
	}
	for (i = 0; i < imnum; i++) {
		ns = dist(engine);
		imscore[i] += ns;
		//		printf("%d, %f,%f\n", i, imscore[i], ns);
	}

	ge_dis = (double *)malloc(sizeof(double)* (genum - 1));
	im_dis = (double *)malloc(sizeof(double)* imnum);

	// For each gescore
	term1 = term3 = 0.0;
	for (i = 0; i < genum; i++) {
		// Distances between the i-th gescore and the other gescores --> ge_dis
		x = 0;
		for (j = 0; j < genum; j++) {
			if (i == j) continue;
			ge_dis[x++] = fabs(gescore[i] - gescore[j]);
		}
		/*
		fp = FileOpen("debug_gescore.csv", "a");
		x = 0;
		for (j = 0; j < genum; j++){
		if (i == j) continue;
		fprintf(fp, "%d,%f,%f,%f\n", j, gescore[i], gescore[j], ge_dis[x++]);
		}
		fclose(fp);
		*/

		// Sort ge_dis in ascending order of distances
		qsort(ge_dis, genum - 1, sizeof(double), (int(*) (const void *, const void *))compare_double);

		// Distances between the i-th gescore and imscores --> im_dis
		x = 0;
		for (j = 0; j < imnum; j++) {
			im_dis[x++] = fabs(gescore[i] - imscore[j]);
		}
		/*
		fp = FileOpen("debug_imscore.csv", "a");
		x = 0;
		for (j = 0; j < imnum; j++){
		fprintf(fp, "%d,%f,%f,%f\n", j, gescore[i], imscore[j], im_dis[x++]);
		}
		fclose(fp);
		*/

		// Sort im_dis in ascending order of distances
		qsort(im_dis, imnum, sizeof(double), (int(*) (const void *, const void *))compare_double);

		// k-th smallest distance in ge_dis --> rho
		rho = ge_dis[k - 1];
		// k-th smallest distance in im_dis --> nu
		nu = im_dis[k - 1];

		// Update the first term
		if (nu != 0.0 || rho != 0.0) term1 += log(nu / rho);

		//		printf("%d,%f,%f,%f\n", i, rho, nu, term1);
	}
	// Update the first term
	term1 /= (double)genum;
	// Update the third term
	term3 = log((double)imnum / ((double)genum - 1.0));
	// Compute the KL divergence (nat)
	kl = term1 + term3;

	// Transform nat into bit
	kl = kl / log(2.0);

	//	printf("%f,%f\n", term1, term3);
	//	printf("%f\n", kl);

	free(ge_dis);
	free(im_dis);

	return kl;
}

// Calculate the KL divergence based on data dependent partitioning [Wang+, T-IT05]
double KL_DDP(double *gescore, double *imscore, int genum, int imnum) {
	double ns;
	int lm, tm, ki;
	double ylm;
	int gp;
	double dm;
	double kl;
	int i, j;
	FILE *fp;

	double sigma = 0.0001;

	// Initialization
	std::random_device seed_gen;
	std::default_random_engine engine(seed_gen());
	std::normal_distribution<> dist(0.0, sigma);

	// Add noise from the Gaussian distribution N(0,sigma^2) --> gescore, imscore
	for (i = 0; i < genum; i++) {
		ns = dist(engine);
		gescore[i] += ns;
		//		printf("%d,%f,%f\n", i, gescore[i], ns);
	}
	for (i = 0; i < imnum; i++) {
		ns = dist(engine);
		imscore[i] += ns;
		//		printf("%d, %f,%f\n", i, imscore[i], ns);
	}

	// Sort gescore and imscore in ascending order of scores
	qsort(gescore, genum, sizeof(double), (int(*) (const void *, const void *))compare_double);
	qsort(imscore, imnum, sizeof(double), (int(*) (const void *, const void *))compare_double);
	/*
	fp = FileOpen("debug_gescore_noise_sort.csv", "w");
	for (i = 0; i < genum; i++) fprintf(fp, "%d,%f\n", i, gescore[i]);
	fclose(fp);
	fp = FileOpen("debug_imscore_noise_sort.csv", "w");
	for (i = 0; i < imnum; i++) fprintf(fp, "%d,%f\n", i, imscore[i]);
	fclose(fp);
	*/

	//	lm = (int)sqrt((double)imnum);
	lm = 10;
	tm = imnum / lm;
	//	printf("%d,%d\n", lm, tm);

	gp = 0;
	kl = 0.0;
	// For each partition until the (tm - 1)-th partition
	for (i = 0; i < tm - 1; i++) {
		// Y_lm (last imscore) --> ylm
		ylm = imscore[(i + 1) * lm - 1];

		// gscore pointer, ki (number of gscores) --> gp, ki
		ki = 0;
		for (j = gp; j < genum; j++) {
			if (gescore[j] > ylm) break;
		}
		ki = j - gp;
		gp = j;

		if (ki > 0) {
			kl += ((double)ki / (double)genum) * log(((double)ki / (double)genum) / ((double)lm / (double)imnum));
		}
		//		printf("%d,%d,%f,%d,%d,%e\n", i, (i + 1) * lm - 1, ylm, gp, ki, kl);
	}

	// Last partition (the tm-th partition)
	ki = genum - gp;
	gp = genum;
	dm = ((double)imnum - (double)lm * tm) / (double)imnum;
	if (ki > 0) {
		kl += ((double)ki / (double)genum) * log(((double)ki / (double)genum) / (((double)lm / (double)imnum) + dm));
	}
	//	printf("%d,%d,%f,%d,%d,%e,%e\n", i, (i + 1) * lm - 1, ylm, gp, ki, kl, dm);
	kl = kl / log(2.0);

	return kl;
}

int main(int argc, char *argv[])
{
	char inoutdir[129], filename[129];
	int rnd_imscore, iuser_num, kl_est;
	char gescorefile[1025], imscorefile[1025], klfile[1025];
	int un;
	char s[1025];
	double kl;
	int *rank;
	int totgenum, totimnum;
	struct UserScoreType *geuserscore, *imuserscore;
	double *gescore, *imscore;
	int genum, imnum;
	int *rndperm;
	int eval_vuser;
	char *tok;
	int i, j, x;
	FILE *fp;

	// Initialization of Mersennne Twister
	unsigned long init[4] = { 0x123, 0x234, 0x345, 0x456 }, length = 4;
	init_by_array(init, length);

	if (argc < 3) {
		printf("Usage: %s [DataDir] [FileName] ([RndImscore (default:1)] [IUserNum (default:0)] [KLEst (default:0)])\n\n", argv[0]);
		printf("[DataDir]: Data directory\n");
		printf("[FileName]: File name (right before the suffix)\n");
		printf("[RndImscore]: Number of randomly selected impostor scores per user (0: all)\n");
		printf("[IUserNum]: Number of incremented users (0: all)\n");
		printf("[KLEst]: KL estimator(-1: data dependent partitioning [Wang+, T-IT05], 0: generalized kNN estimator [Wang+, T-IT09], k(>=1): kNN estimator [Wang+, T-IT09])\n\n");

		return -1;
	}

	sprintf(inoutdir, argv[1]);
	sprintf(filename, argv[2]);

	rnd_imscore = 1;
	if (argc >= 4) rnd_imscore = atoi(argv[3]);
	iuser_num = 0;
	if (argc >= 5) iuser_num = atoi(argv[4]);
	kl_est = 0;
	if (argc >= 6) kl_est = atoi(argv[5]);
	printf("%s,%s,%d,%d,%d\n", inoutdir, filename, rnd_imscore, iuser_num, kl_est);

	// gescorefile, imscorefile, klfile
	sprintf(gescorefile, "%s/ge_score_%s.csv", inoutdir, filename);
	sprintf(imscorefile, "%s/im_score_%s_r%d.csv", inoutdir, filename, rnd_imscore);
	sprintf(klfile, "%s/kl_%s_r%d_es%d.csv", inoutdir, filename, rnd_imscore, kl_est);

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
	geuserscore = (struct UserScoreType *)malloc(sizeof(struct UserScoreType) * totgenum);
	imuserscore = (struct UserScoreType *)malloc(sizeof(struct UserScoreType) * totimnum);
	rank = (int *)malloc(sizeof(int) * totgenum);
	rndperm = (int *)malloc(sizeof(int) * totgenum);
	gescore = (double *)malloc(sizeof(double) * totgenum);
	imscore = (double *)malloc(sizeof(double) * totimnum);

	// Read genuine scores and ranks --> geuserscore, rank
	printf("Reading genuine scores... ");
	fp = FileOpen(gescorefile, "r");
	fgets(s, 1024, fp);
	for (i = 0; i < totgenum; i++) {
		fgets(s, 1024, fp);
		tok = strtok(s, ",");
		geuserscore[i].vuser = atoi(tok);
		tok = strtok(NULL, ",");
		tok = strtok(NULL, ",");
		geuserscore[i].score = atof(tok);
		tok = strtok(NULL, ",");
		tok[strlen(tok) - 1] = '\0';
		rank[i] = atoi(tok);
		//		printf("%d,%d,%d,%f,%d\n", i, geuserscore[i].vuser, geuserscore[i].euser, geuserscore[i].score, rank[i]);
	}
	fclose(fp);
	printf("done.\n");
	printf("#gescores: %d\n", totgenum);

	// Read impostor scores --> imuserscore
	printf("Reading impostor scores... ");
	fp = FileOpen(imscorefile, "r");
	fgets(s, 1024, fp);
	for (i = 0; i < totimnum; i++) {
		fgets(s, 1024, fp);
		tok = strtok(s, ",");
		imuserscore[i].vuser = atoi(tok);
		tok = strtok(NULL, ",");
		tok = strtok(NULL, ",");
		imuserscore[i].score = atof(tok);
		//		printf("%d,%d,%d,%f\n", i, imuserscore[i].vuser, imuserscore[i].euser, imuserscore[i].score);
	}
	fclose(fp);
	printf("done.\n");
	printf("#imscores: %d\n", totimnum);

	// Randomly generate 0, 1, 2, ..., totgenum-1 --> rndperm
	MakeRndPerm(rndperm, totgenum);

	// Output the KL divergence file
	fp = FileOpen(klfile, "w");
	fprintf(fp, "genum,imnum,kl\n");

	// Evaluate the KL divergence for each number of users
	printf("Evaluating the KL divergence:\n");
	if (iuser_num == 0) iuser_num = totgenum;
//	for (un = iuser_num; un <= totgenum; un += iuser_num) {
	for (un = iuser_num; un < totgenum + iuser_num; un += iuser_num) {
		if (un > totgenum) un = totgenum;
		// Genuine scores --> gescore
		x = 0;
		for (i = 0; i < totgenum; i++) {
			eval_vuser = 0;
			for (j = 0; j < un; j++) {
				if (geuserscore[i].vuser == rndperm[j]) {
					eval_vuser = 1;
					break;
				}
			}
			if (eval_vuser == 1) gescore[x++] = geuserscore[i].score;
		}
		// Number of genuine scores --> genum
		genum = x;

		// Impostor scores --> imscore
		x = 0;
		for (i = 0; i < totimnum; i++) {
			eval_vuser = 0;
			for (j = 0; j < un; j++) {
				if (imuserscore[i].vuser == rndperm[j]) {
					eval_vuser = 1;
					break;
				}
			}
			if (eval_vuser == 1) imscore[x++] = imuserscore[i].score;
		}
		// Number of impostor scores --> imnum
		imnum = x;

		// Calculate the KL divergence
		// kNN estimator [Wang+, T-IT09]
		if (kl_est >= 1) {
			kl = KL_kNN(gescore, imscore, genum, imnum, kl_est);
		}
		// generalized kNN estimator [Wang+, T-IT09]
		else if (kl_est == 0) {
			kl = KL_GenNN(gescore, imscore, genum, imnum);
		}
		// data dependent partitioning [Wang+, T-IT05]
		else if (kl_est == -1) {
			kl = KL_DDP(gescore, imscore, genum, imnum);
		}
		else {
			printf("Wrong KL_EST.\n");
			exit(-1);
		}

		printf("genum:%d, imnum: %d, kl: %f\n", genum, imnum, kl);
		fprintf(fp, "%d,%d,%f\n", genum, imnum, kl);
	}

	fclose(fp);

	// free
	free(geuserscore);
	free(imuserscore);
	free(rank);
	free(rndperm);
	free(gescore);
	free(imscore);

	return 0;
}

