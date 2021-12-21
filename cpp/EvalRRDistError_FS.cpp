#define _USE_MATH_DEFINES
#include <iostream>
#include <sstream>
#include <iomanip>
#include <stdexcept>
#include <cmath>
#include <map>
#include <tuple>
#include <stdio.h>
#include <string.h>
#include <stdlib.h>
//#include <math.h>
#include "mt19937ar2.h"

using namespace std;

struct DistType{
	double dist;
	int index;
};

FILE *FileOpen(const char *filename, const char *mode) {
	FILE *fp;

	if ((fp = fopen(filename, mode)) == NULL) {
		printf("cannot open %s\n", filename);
		exit(-1);
	}
	return fp;
}

int compare_DistType(struct DistType *x, struct DistType *y) {
	if (x->dist < y->dist)       return(1);   /* return positive integer */
	else if (x->dist == y->dist) return(0);   /* return zero     integer */
	else              return(-1);  /* return negative integer */
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

// Computation of the inverse of the normal CDF by John D. Cook:
// https://www.johndcook.com/blog/cpp_phi_inverse/
double RationalApproximation(double t)
{
    // Abramowitz and Stegun formula 26.2.23.
    // The absolute value of the error should be less than 4.5 e-4.
    double c[] = {2.515517, 0.802853, 0.010328};
    double d[] = {1.432788, 0.189269, 0.001308};
    return t - ((c[2]*t + c[1])*t + c[0]) / 
               (((d[2]*t + d[1])*t + d[0])*t + 1.0);
}

// Computation of the inverse of the normal CDF by John D. Cook:
// https://www.johndcook.com/blog/cpp_phi_inverse/
double NormalCDFInverse(double p)
{
    if (p <= 0.0 || p >= 1.0)
    {
        std::stringstream os;
        os << "Invalid input argument (" << p 
           << "); must be larger than 0 but less than 1.";
        throw std::invalid_argument( os.str() );
    }

    // See article above for explanation of this section.
    if (p < 0.5)
    {
        // F^-1(p) = - G^-1(p)
        return -RationalApproximation( sqrt(-2.0*log(p)) );
    }
    else
    {
        // F^-1(p) = G^-1(1-p)
        return RationalApproximation( sqrt(-2.0*log(1-p)) );
    }
}

// Estimate odist from edist using empirical estimation with thresholding. 
void EmpEst(double *odist_est, double *edist, int euser_num, int alp_num, double eps, double *phi_inv, double *sd, double *thr, int est_mtd){
//	double phi_inv, sd, thr;
	int zero_alp_num;
	double odist_est_sum;
	int i;

	// ****************************** Empirical Estimation ****************************** //
	// Empirical estimate --> odist_est
	for (i = 0; i < alp_num; i++){
		if (eps != -1) odist_est[i] = (exp(eps) + alp_num - 1) / (exp(eps) - 1) * edist[i] - 1.0 / (exp(eps) - 1);
		else odist_est[i] = edist[i];
	}

	// ****** Thresholding [Erlingsson+,CCS14][Wang+,USENIX17][Murakami+,USENIX19] ****** //
	if (est_mtd == 0 && eps != -1){
		// Use a value computed from NORM.INV in excel --> (*phi_inv)
		if(alp_num == 10500393) (*phi_inv) = 5.739005;
		// Use aan approximate value by John D. Cook --> (*phi_inv)
		else {
			// Phi^{-1}(1 - 0.05/alp_num) --> (*phi_inv)
			(*phi_inv) = NormalCDFInverse(1 - 0.05/(double)alp_num);
			printf("Use approximate (*phi_inv) by John D. Cook\n");
		}
		// standard deviation --> (*sd)
		(*sd) = sqrt(((double)alp_num + exp(eps) - 2.0) / (double)euser_num) / (exp(eps) - 1.0);
		// threshold --> (*thr)
		(*thr) = (*phi_inv) * (*sd);
//		printf("phi_inv:%f, sd:%e, thr:%e\n", (*phi_inv), (*sd), (*thr));
		// Assign zero if the probability in odist_est is <= (*thr)
		zero_alp_num = 0;
		for (i = 0; i < alp_num; i++){
			if (odist_est[i] <= (*thr)){
				odist_est[i] = 0.0;
				zero_alp_num++;
			}
		}

		/*
		// Sum of the probabilities in odist_est
		odist_est_sum = 0.0;
		for (i = 0; i < alp_num; i++) odist_est_sum += odist_est[i];
		// If the sum is less than 1, uniformly assign non-zero values to zero-values in odist_est 
//		if (odist_est_sum < 1.0){
//			for (i = 0; i < alp_num; i++){
//				if (odist_est[i] == 0) odist_est[i] = (1.0 - odist_est_sum) / zero_alp_num;
//			}
//		}
		// Normalize odist_est so that the sum is 1
		for (i = 0; i < alp_num; i++) odist_est[i] /= odist_est_sum;
		*/
	}

}

// Calculate the l1-loss between dist1 and dist2 for the top-K whose values in dist1 are the highest
double L1TopK(double *dist1, double *dist2, struct DistType *odist_sort, int alp_num, int k){
//	struct DistType *dist1_sort;
	double l1topk = 0.0;
	double l1_loss;
	int i, j;

	/*
	dist1_sort = (struct DistType *)malloc(sizeof(struct DistType)* alp_num);
	for (i = 0; i < alp_num; i++){
		dist1_sort[i].dist = dist1[i];
		dist1_sort[i].index = i;
	}
	qsort(dist1_sort, alp_num, sizeof(struct DistType), (int(*) (const void *, const void *))compare_DistType);
//	for (i = 0; i < k; i++) printf("%d,%f,%d\n",i,dist1_sort[i].dist,dist1_sort[i].index);
	*/

	for (i = 0; i < k; i++){
//		j = dist1_sort[i].index;
		j = odist_sort[i].index;
		l1_loss = fabs(dist1[j] - dist2[j]);
		l1topk += l1_loss;
//		printf("%d,%d,%e,%e,%e\n",i,j,dist1[j],dist2[j],l1_loss);
	}

//	free(dist1_sort);
	return l1topk;
}

// Calculate the l2-loss between dist1 and dist2 for the top-K whose values in dist1 are the highest
double L2TopK(double *dist1, double *dist2, struct DistType *odist_sort, int alp_num, int k){
//	struct DistType *dist1_sort;
	double l2topk = 0.0;
	double l2_loss;
	int i, j;

	/*
	dist1_sort = (struct DistType *)malloc(sizeof(struct DistType)* alp_num);
	for (i = 0; i < alp_num; i++){
		dist1_sort[i].dist = dist1[i];
		dist1_sort[i].index = i;
	}
	qsort(dist1_sort, alp_num, sizeof(struct DistType), (int(*) (const void *, const void *))compare_DistType);
//	for (i = 0; i < k; i++) printf("%d,%f,%d\n",i,dist1_sort[i].dist,dist1_sort[i].index);
	*/

	for (i = 0; i < k; i++){
//		j = dist1_sort[i].index;
		j = odist_sort[i].index;
		l2_loss = (dist1[j] - dist2[j]) * (dist1[j] - dist2[j]);
		l2topk += l2_loss;
//		printf("%d,%d,%e,%e,%e\n",i,j,dist1[j],dist2[j],l2_loss);
	}

//	free(dist1_sort);
	return l2topk;
}

// Calculate TV between dist1 and dist2
double TotalVariation(double *dist1, double *dist2, int alp_num){
	double tv = 0.0;
	double l1_loss;
	int i;

	for (i = 0; i < alp_num; i++){
		l1_loss = fabs(dist1[i] - dist2[i]);
		tv += l1_loss;
	}
	tv /= 2.0;

	return tv;
}

// Calculate MSE between dist1 and dist2
double MSE(double *dist1, double *dist2, int alp_num){
	double mse = 0.0;
	double l2_loss;
	int i;

	for (i = 0; i < alp_num; i++){
		l2_loss = (dist1[i] - dist2[i]) * (dist1[i] - dist2[i]);
		mse += l2_loss;
	}

	return mse;
}

// Calculate the JS divergence between dist1 and dist2
double JSDivergence(double *dist1, double *dist2, int alp_num){
	double *dist_avg;
	double js;
	double kl1 = 0.0;
	double kl2 = 0.0;
	int i;

	// Malloc
	dist_avg = (double *)malloc(sizeof(double)* alp_num);

	for (i = 0; i < alp_num; i++) dist_avg[i] = (dist1[i] + dist2[i]) / 2.0;

	// kl1
	for (i = 0; i < alp_num; i++){
		if (dist1[i] == 0.0) continue;
		kl1 += dist1[i] * log(dist1[i] / dist_avg[i]);
	}
	kl1 /= 2.0;

	// kl2
	for (i = 0; i < alp_num; i++){
		if (dist2[i] == 0.0) continue;
		kl2 += dist2[i] * log(dist2[i] / dist_avg[i]);
	}
	kl2 /= 2.0;

	// JS divergence (= kl1 + kl2)
	js = kl1 + kl2;

	// Free
	free(dist_avg);

	return js;
}

int main(int argc, char *argv[])
{
	char outdir[129], city[129];
	int alp_num, trace_len, top_k;
	char eps_s[129];
	double eps;
	double phi_inv, sd, thr;

	char otrace_file[129], etrace_file[129];
	double *odist, *edist, *odist_est;
	double odist_sum, edist_sum, odist_est_sum;
	int euser_num;

	double odist_est_l1topk, odist_est_l2topk, odist_est_tv, odist_est_mse, odist_est_js;
	double edist_l1topk, edist_l2topk, edist_tv, edist_mse, edist_js;

	struct DistType *odist_sort;

	int poi_index;
	int data_num;
	char outfile[129];
	char s[1024];
	char *tok;
	int i, j;
	FILE *fp;

	int est_mtd;

	// Initialization of Mersennne Twister
	unsigned long init[4] = { 0x123, 0x234, 0x345, 0x456 }, length = 4;
	init_by_array(init, length);

	if (argc < 7) {
		printf("Usage: %s [DataDir] [City] [#Alp] [Epsilon] [TraceLen] [K] ([EstMtd (default:0)])\n\n", argv[0]);
		printf("[DataDir]: Data directory\n");
		printf("[City]: City (AL/IS/JK/KL/NY/SP/TK)\n");
		printf("[#Alp]: Number of alphabets\n");
		printf("[Epsilon]: Epsilon (-1: no noise)\n");
		printf("[TraceLen]: Trace length\n");
		printf("[K]: Top-K in the original distribution\n");
		printf("[EstMtd]: Estimation method (0: empirical + threshold, 1: empirical)\n");
		return -1;
	}

	sprintf(outdir, argv[1]);
	sprintf(city, argv[2]);
	alp_num = atoi(argv[3]);
	strcpy(eps_s, argv[4]);
	eps = atof(eps_s);
	trace_len = atoi(argv[5]);
	top_k = atoi(argv[6]);

	est_mtd = 0;
	if (argc >= 8) est_mtd = atoi(argv[7]);

	// original trace --> otrace_file
//	sprintf(otrace_file, "%s/testtraces.csv", outdir, city);
	sprintf(otrace_file, "%s/testtraces_%s_RR-e-1-l%d.csv", outdir, city, trace_len);
	// testing trace (obfuscated trace) --> etrace_file
	sprintf(etrace_file, "%s/testtraces_%s_RR-e%s-l%d.csv", outdir, city, eps_s, trace_len);
	// result file --> outfile
	if (est_mtd == 0) sprintf(outfile, "%s/disterr_%s_RR-e%s-l%d-k%d.csv", outdir, city, eps_s, trace_len, top_k);
	else sprintf(outfile, "%s/disterr_%s_RR-e%s-l%d-k%d-m%d.csv", outdir, city, eps_s, trace_len, top_k, est_mtd);

	printf("%s,%s,%d,%f,%d\n", outdir, city, alp_num, eps, trace_len);

	printf("[Original]: %s\n", otrace_file);
	printf("[Testing (Obfuscated)]: %s\n", etrace_file);

	// Malloc
	odist = (double *)malloc(sizeof(double) * alp_num);
	edist = (double *)malloc(sizeof(double) * alp_num);
	odist_est = (double *)malloc(sizeof(double) * alp_num);

	// Initialization
	for (i = 0; i < alp_num; i++){
		odist[i] = 0;
		edist[i] = 0;
	}

	// Calculate a distribution from original traces --> odist
	fp = FileOpen(otrace_file, "r");
	data_num = 0;
	euser_num = 0;
	while (fgets(s, 1024, fp) != NULL) {
		strtok(s, ",");
		tok = strtok(NULL, ",");
		poi_index = atoi(tok);
		odist[poi_index]++;
		data_num++;
		euser_num++;
	}
	fclose(fp);
	for (i = 0; i < alp_num; i++) odist[i] /= (double)data_num;
	odist_sum = 0.0;
	for (i = 0; i < alp_num; i++) odist_sum += odist[i];

	// Calculate a distribution from testing (obfuscated) traces --> edist
	fp = FileOpen(etrace_file, "r");
	data_num = 0;
	while (fgets(s, 1024, fp) != NULL) {
		strtok(s, ",");
		tok = strtok(NULL, ",");
		poi_index = atoi(tok);
		edist[poi_index]++;
		data_num++;
	}
	fclose(fp);
	for (i = 0; i < alp_num; i++) edist[i] /= (double)data_num;
	edist_sum = 0.0;
	for (i = 0; i < alp_num; i++) edist_sum += edist[i];

	// Estimate odist from edist using empirical estimation with thresholding --> odist_est
	EmpEst(odist_est, edist, euser_num, alp_num, eps, &phi_inv, &sd, &thr, est_mtd);
	odist_est_sum = 0.0;
	for (i = 0; i < alp_num; i++) odist_est_sum += odist_est[i];
//	printf("odist_sum: %f, edist_sum: %f, odist_est_sum: %f\n", odist_sum, edist_sum, odist_est_sum);

	// Sort odist in descending order of probabilities --> odist_sort
	odist_sort = (struct DistType *)malloc(sizeof(struct DistType)* alp_num);
	for (i = 0; i < alp_num; i++){
		odist_sort[i].dist = odist[i];
		odist_sort[i].index = i;
	}
	qsort(odist_sort, alp_num, sizeof(struct DistType), (int(*) (const void *, const void *))compare_DistType);

	// Calculate the L1-loss of odist_est for the top-k --> odist_est_l1topk
	odist_est_l1topk = L1TopK(odist, odist_est, odist_sort, alp_num, top_k);
	// Calculate the L2-loss of odist_est for the top-k --> odist_est_l2topk
	odist_est_l2topk = L2TopK(odist, odist_est, odist_sort, alp_num, top_k);

	// Calculate TV of odist_est --> odist_est_tv
	odist_est_tv = TotalVariation(odist, odist_est, alp_num);
	// Calculate MSE of odist_est --> odist_est_mse
	odist_est_mse = MSE(odist, odist_est, alp_num);
	// Calculate MSE of odist_est --> odist_est_js
	odist_est_js = JSDivergence(odist, odist_est, alp_num);

	// Calculate the L1-loss of edist for the top-k --> edist_l1topk
	edist_l1topk = L1TopK(odist, edist, odist_sort, alp_num, top_k);
	// Calculate the L2-loss of edist for the top-k --> edist_l2topk
	edist_l2topk = L2TopK(odist, edist, odist_sort, alp_num, top_k);

	// Calculate TV of edist --> edist_tv
	edist_tv = TotalVariation(odist, edist, alp_num);
	// Calculate MSE of edist --> edist_mse
	edist_mse = MSE(odist, edist, alp_num);
	// Calculate MSE of edist --> edist_js
	edist_js = JSDivergence(odist, edist, alp_num);

	// Output the results
	fp = FileOpen(outfile, "w");
	fprintf(fp, "K,L1TopK(odist_est),L2TopK(odist_est),TV(odist_est),MSE(odist_est),JS(odist_est)\n");
	fprintf(fp, "%d,%e,%e,%e,%e,%e\n", top_k, odist_est_l1topk, odist_est_l2topk, odist_est_tv, odist_est_mse, odist_est_js);
	fprintf(fp, "K,L1TopK(edist),L2TopK(edist),TV(edist),MSE(edist),JS(edist)\n");
	fprintf(fp, "%d,%e,%e,%e,%e,%e\n", top_k, edist_l1topk, edist_l2topk, edist_tv, edist_mse, edist_js);
	fprintf(fp, "phi_inv,sd,thr\n");
	fprintf(fp, "%e,%e,%e\n", phi_inv, sd, thr);
	fprintf(fp, "order,poi_index,odist,edist,odist_est\n");
//	for (i = 0; i < alp_num; i++) fprintf(fp, "%d,%e,%e,%e\n", i, odist[i], edist[i], odist_est[i]);
	for (i = 0; i < top_k; i++){
		j = odist_sort[i].index;
		fprintf(fp, "%d,%d,%e,%e,%e\n", i, j, odist[j], edist[j], odist_est[j]);
	}
	fclose(fp);

	// Free
	free(odist);
	free(edist);
	free(odist_est);
	free(odist_sort);
	return 0;
}
