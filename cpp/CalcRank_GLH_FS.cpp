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

// Check whether each user provides the required number of locations
void CheckUser(char *ttrace_file, map<int, int> &meet_req, int req_num_loc, int num_loc) {
	map<int, int> cnt;
	map<int, int>::iterator citr;
	int user_index, user_index_prev;
	char s[1025];
//	char s2[1025];
	FILE *fp;

	// Open the training trace file
	fp = FileOpen(ttrace_file, "r");

	// Skip the header
	fgets(s, 1024, fp);

	// Count the number of visits for each user --> cnt
	user_index_prev = -1;
	while (fgets(s, 1024, fp) != NULL) {
//		strcpy_s(s2, s);
		if (strtok(s, ",") == NULL) continue;
		user_index = atoi(s);
//		if (user_index == 0) printf("%s", s2);
		cnt[user_index] += 1;
		user_index_prev = user_index;
	}
	fclose(fp);

	// Check whether each user provides the required number of locations --> meet_req
	for (citr = cnt.begin(); citr != cnt.end(); citr++) {
		user_index = citr->first;
		if (cnt[user_index] >= req_num_loc) {
			meet_req[user_index] = 1;
//			cout << user_index << " " << cnt[user_index] << " " << meet_req[user_index] << endl;
		}
		else {
			meet_req[user_index] = 0;
		}
	}
}

// Read a training trace file
void ReadTTraceFile(char *ttrace_file, map<int, int> &meet_req, map<int, double> *visit_vec, map<tuple<int, int>, double> *trans_mat, 
					map<int, tuple<double, double>> &poi_info, int num_loc, map<int, int> poi2code) {
	map<int, double>::iterator vitr;
	map<tuple<int, int>, double>::iterator titr;
	map<int, int> visit_sum;
	map<tuple<int, int>, int> trans_sum;
	int user_index, user_index_prev;
	int poi_index, poi_index_prev;
	double y, x;
	int nl;
	int user_num;
	int i;
	char s[1025];
	FILE *fp;
	char *tok;

	int cindex, cindex_prev;

	// Open the training trace file
	fp = FileOpen(ttrace_file, "r");

	// Skip the header
	fgets(s, 1024, fp);

	// Count the number of visits, transitions, locations (from) --> visit_vec, trans_mat, trans_sum
	user_index_prev = -1;
	poi_index_prev = -1;
	nl = 0;
	user_num = 0;
	while (fgets(s, 1024, fp) != NULL) {
		strtok(s, ",");
		user_index = atoi(s);

		// Continue if user_index does not provide the required number of locations
		if (meet_req[user_index] == 0) continue;
		// Continue if the number of locations reaches num_loc
		if (user_index != user_index_prev) nl = 0;
		if (num_loc != 0 && nl >= num_loc) continue;

		tok = strtok(NULL, ",");
		poi_index = atoi(tok);
//		printf("%d,%d,%d\n", user_index, poi_index, user_num);

		cindex = poi2code[poi_index];

		tok = strtok(NULL, ",");
		if(tok == NULL) y = 0;
		else y = atof(tok);

		tok = strtok(NULL, ",");
		if(tok == NULL) x = 0;
		else x = atof(tok);

		// POI information --> poi_info
		poi_info[poi_index] = make_tuple(y, x);

		if (user_index_prev != -1 && user_index != user_index_prev) {
			user_num++;
//			cout << user_num << endl;
		}

		visit_vec[user_num][cindex] += 1;
		visit_sum[user_num] += 1;
		if (user_index == user_index_prev) {
			trans_mat[user_num][make_tuple(cindex_prev, cindex)] += 1;
			trans_sum[make_tuple(user_num, cindex_prev)] += 1;
		}

		user_index_prev = user_index;
		poi_index_prev = poi_index;
		cindex_prev = cindex;
		nl++;
	}
	fclose(fp);

	// For each user
	for (i = 0; i < user_num; i++) {
		// Normalize visit_vec[i]
		for (vitr = visit_vec[i].begin(); vitr != visit_vec[i].end(); vitr++) {
			visit_vec[i][vitr->first] /= (double)visit_sum[i];
		}
		// Normalize trans_mat[i]
		for (titr = trans_mat[i].begin(); titr != trans_mat[i].end(); titr++) {
			cindex_prev = get<0>(titr->first);
			trans_mat[i][titr->first] /= (double)trans_sum[make_tuple(i, cindex_prev)];
		}
	}
}

// Read a testing trace file
void ReadETraceFile(char *etrace_file, map<int, int> &meet_req, map<int, int> *etrace, map<int, int> poi2code) {
	int user_index, user_index_prev;
	int poi_index;
	int time_ins;
	int user_num;
	char s[1025];
	FILE *fp;
	char *tok;

	int cindex;

	// Open the testing trace file
	fp = FileOpen(etrace_file, "r");
	// Skip the header
	fgets(s, 1024, fp);

	// Read testing traces --> etrace
	user_index_prev = -1;
	time_ins = 0;
	user_num = 0;
	while (fgets(s, 1024, fp) != NULL) {
		strtok(s, ",");
		user_index = atoi(s);

		// Continue if user_index does not provide the required number of locations
		if (meet_req[user_index] == 0) continue;

		tok = strtok(NULL, ",");
		cindex = atoi(tok);

		if (user_index_prev != -1 && user_index != user_index_prev) {
			user_num++;
			time_ins = 0;
		}

		// Read a testing event
		etrace[user_num][time_ins] = cindex;

		user_index_prev = user_index;
		time_ins++;
	}

	fclose(fp);
}

// Calculate a score (sum of (loglikelihood - logdelta)) using a transition matrix
double CalcScoreTransMat(map<int, int> etrace, map<int, double> visit_vec, map<tuple<int, int>, double> trans_mat) {
	map<int, int>::iterator eitr;
	double score = 0.0;
	double logdelta = log(Delta);
	int time_ins;
	int cindex, cindex_prev;

	for (eitr = etrace.begin(); eitr != etrace.end(); eitr++) {
		time_ins = eitr->first;
		cindex = eitr->second;
		if (time_ins == 0) {
			if (visit_vec[cindex] > 0) {
//				score += log(visit_vec[cindex]);
				score += (log(visit_vec[cindex]) - logdelta);
			}
//			else {
//				score += logdelta;
//			}
		}
		else {
			if (trans_mat[make_tuple(cindex_prev, cindex)] > 0) {
//				score += log(trans_mat[make_tuple(cindex_prev, cindex)]);
				score += (log(trans_mat[make_tuple(cindex_prev, cindex)]) - logdelta);
			}
//			else {
//				score += logdelta;
//			}
		}
		cindex_prev = cindex;
	}

	return score;
}

// Calculate a score (sum of (loglikelihood - logdelta) using a visiting probability vector
double CalcScoreVisitVec(map<int, int> etrace, map<int, double> visit_vec) {
	map<int, int>::iterator eitr;
	double score = 0.0;
	double logdelta = log(Delta);
	int time_ins;
	int cindex;

	for (eitr = etrace.begin(); eitr != etrace.end(); eitr++) {
		time_ins = eitr->first;
		cindex = eitr->second;
		if (visit_vec[cindex] > 0) {
//			score += log(visit_vec[cindex]);
			score += (log(visit_vec[cindex]) - logdelta);
		}
//		else {
//			score += logdelta;
//		}
	}

	return score;
}

// Compute the density of the Gaussian distribution with zero-mean
double Gaussian(double x, double sta_dev){
	double density;

	density = exp((- x * x) / (2.0 * sta_dev * sta_dev)) / (sqrt(2.0 * M_PI) * sta_dev);

	return density;
}

int main(int argc, char *argv[])
{
	char outdir[129], city[129], obf[129];
	int adv_know, reid_method, req_num_loc, num_loc, st_user, en_user;
	int rank,same_score_num;
	char ttrace_file[129], otrace_file[129], etrace_file[129];
	map<int, int> meet_req;
	map<int, double> *visit_vec;
	map<tuple<int, int>, double> *trans_mat;
	map<int, int> *etrace;
	map<int, tuple<double, double>> poi_info;
	map<int, int>::iterator citr;
	map<int, double>::iterator vitr;
	map<tuple<int, int>, double>::iterator titr;
	map<int, int>::iterator eitr;
	map<int, tuple<double, double>>::iterator pitr;
	int muser_num;
	int *muser;
	int *rndperm;
	int rndindex;
	double loglikeli;
	double score, score2;
	char outfile[129];
	int i, j;
	FILE *fp;

	int m, g;
	char etrace_h_file[129], etrace_c_file[129];
	map<int, int> poi2code;
	map<int, int>::iterator codeitr;
	int poi_index, cindex;
	char s[1025];
	char *tok;

	// Initialization of Mersennne Twister
	unsigned long init[4] = { 0x123, 0x234, 0x345, 0x456 }, length = 4;
	init_by_array(init, length);

	if (argc < 4) {
		printf("Usage: %s [DataDir] [City] [Obf] ([AdvKnow (default:2)] [ReidMethod (default:1)] [ReqNumLoc (default:1)] [NumLoc (default:0)] [StUser (default:0)] [EnUser (default:0)])\n\n", argv[0]);
		printf("[DataDir]: Data directory\n");
		printf("[City]: City (AL/IS/JK/KL/NY/SP/TK)\n");
		printf("[Obf]: Obfuscation (GLH-e[Epsilon]-m1-g[g]-l[TraceLen])\n");
		printf("[AdvKnow]: Adversary's knowledge (1: partial knowledge, 2: maximum knowledge (training = original), 3: maximum knowledge (training = obfuscated))\n");
		printf("[ReidMethod]: Re-identification method (1:transition matrix, 2:visiting probability vector)\n");
		printf("[ReqNumLoc]: Required number of training locations per user (used for pruning users)\n");
		printf("[NumLoc]: Number of training locations per user (0: all)\n");
		printf("[StUser]: Start user index\n");
		printf("[EnUser]: End user index (0: last user index)\n");
		return -1;
	}

	sprintf(outdir, argv[1]);
	sprintf(city, argv[2]);
	sprintf(obf, argv[3]);

	// training trace --> ttrace_file
	sprintf(ttrace_file, "%s/traintraces_%s.csv", outdir, city);
	// original trace --> otrace_file
	sprintf(otrace_file, "%s/testtraces_%s.csv", outdir, city);
	// testing trace (obfuscated trace) --> etrace_file
	sprintf(etrace_file, "%s/testtraces_%s.csv", outdir, city);
	if (strcmp(obf, "org") != 0) sprintf(etrace_file, "%s/testtraces_%s_%s.csv", outdir, city, obf);

	// hash file --> etrace_h_file
	sprintf(etrace_h_file, "%s/testtraces_%s_%s_h.csv", outdir, city, obf);
	// code file --> etrace_c_file
	sprintf(etrace_c_file, "%s/testtraces_%s_%s_c.csv", outdir, city, obf);

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
//	printf("%s,%s,%d,%d,%d,%d\n", outdir, city, adv_know, reid_method, req_num_loc, num_loc);

	// Category size after encoding --> g
	sscanf(obf, "GLH-e%*f-m%d-g%d-l%*d", &m, &g);
	printf("[m]: %d\n", m);
	printf("[g]: %d\n", g);
	if(m != 1){
		printf("The attack in this code assumes that m (#Hashes) = 1.\n");
		exit(-1);
	}

	// Check whether each user provides the required number of training locations
	CheckUser(ttrace_file, meet_req, req_num_loc, num_loc);
	/*
	for (citr = meet_req.begin(); citr != meet_req.end(); citr++) {
		if (citr->second == 1) cout << citr->first << " " << citr->second << endl;
	}
	*/

	switch (adv_know) {
	case 1:	// partial knowledge
		break;
	case 2:	// maximum knowledge (training = original)
		strcpy(ttrace_file, otrace_file);
		break;
	case 3:	// maximum knowledge (training = obfuscated)
		strcpy(ttrace_file, etrace_file);
		break;
	default:
		printf("Wrong ADV_KNOW.\n");
		exit(-1);
	}

	printf("[Training]: %s\n", ttrace_file);
	printf("[Testing]: %s\n", etrace_file);
	if (reid_method == 1) printf("[Re-identification]: transition matrix\n");
	else if (reid_method == 2) printf("[Re-identification]: visiting probability vector\n");
	else{
		printf("Wrong REID_METHOD.\n");
		exit(-1);
	}

	// Number of users who meet the requirement --> muser_num
	muser_num = 0;
	for (citr = meet_req.begin(); citr != meet_req.end(); citr++) {
		if (citr->second == 1) muser_num++;
	}
	printf("muser_num: %d\n", muser_num);

	// Last user index --> en_user
	if (en_user == 0) en_user = muser_num - 1;

	// Users who meet the requirement --> muser
	muser = (int *)malloc(sizeof(int) * muser_num);
	i = 0;
	for (citr = meet_req.begin(); citr != meet_req.end(); citr++) {
		if (citr->second == 1) muser[i++] = citr->first;
	}
//	for (i = 0; i < muser_num; i++) printf("%d,%d\n", i, muser[i]);

	// Read a code file --> poi2code
	fp = FileOpen(etrace_c_file, "r");
	fgets(s, 1024, fp);
	while (fgets(s, 1024, fp) != NULL) {
		strtok(s, ",");
		tok = strtok(NULL, ",");
		poi_index = atoi(tok);
		tok = strtok(NULL, ",");
		cindex = atoi(tok);
		poi2code[poi_index] = cindex;
	}
	fclose(fp);
	/*
	for (codeitr = poi2code.begin(); codeitr != poi2code.end(); codeitr++) {
		printf("%d,%d\n", codeitr->first, codeitr->second);
	}
	*/

	// Read a training trace file
	visit_vec = new map<int, double>[muser_num];
	trans_mat = new map<tuple<int, int>, double>[muser_num];
	ReadTTraceFile(ttrace_file, meet_req, visit_vec, trans_mat, poi_info, num_loc, poi2code);
	/*
	for (i = 0; i < muser_num; i++) {
		for (vitr = visit_vec[i].begin(); vitr != visit_vec[i].end(); vitr++) {
			cout << i << " " << muser[i] << " " << vitr->first << " " << vitr->second << endl;
		}
		for (titr = trans_mat[i].begin(); titr != trans_mat[i].end(); titr++) {
			cout << i << " " << muser[i] << " " << get<0>(titr->first) << " " << get<1>(titr->first) << " " << titr->second << endl;
		}
	}
	for (i = 0; i < 100; i++) cout << i << " " << get<0>(poi_info[i]) << " " << get<1>(poi_info[i]) << endl;
	for(pitr = poi_info.begin(); pitr != poi_info.end(); pitr++){
		cout << pitr->first << " " << get<0>(pitr->second) << " " << get<1>(pitr->second) << endl;
	}
	exit(0);
	*/

	// Read a testing trace file
	etrace = new map<int, int>[muser_num];
	ReadETraceFile(etrace_file, meet_req, etrace, poi2code);
	/*
	for (i = 0; i < muser_num; i++) {
		for (eitr = etrace[i].begin(); eitr != etrace[i].end(); eitr++) {
			cout << i << " " << muser[i] << " " << eitr->first << " " << eitr->second << endl;
		}
	}
	*/

	// Output genuine scores and ranks
	printf("Calculating genuine scores and ranks: ");
	sprintf(outfile, "%s/ge_score-rank_%s_ak%d_rm%d_rnl%d_nl%d_u%d-%d.csv", outdir, city, adv_know, reid_method, req_num_loc, num_loc, st_user, en_user);
	if (strcmp(obf, "org") != 0) sprintf(outfile, "%s/ge_score-rank_%s_%s_ak%d_rm%d_rnl%d_nl%d_u%d-%d.csv", outdir, city, obf, adv_know, reid_method, req_num_loc, num_loc, st_user, en_user);

	fp = FileOpen(outfile, "w");
	fprintf(fp, "vuser,euser,score,rank,same_score_num\n");
//	for (i = 0; i < muser_num; i++) {
	for (i = st_user; i <= en_user; i++) {
		if (i % 1000 == 0) printf("#");
		// Calculate a genuine score of user i
		switch (reid_method) {
		case 1:	// transition matrix
			loglikeli = CalcScoreTransMat(etrace[i], visit_vec[i], trans_mat[i]);
			score = loglikeli;
			break;
		case 2:	// visiting probability vector
			loglikeli = CalcScoreVisitVec(etrace[i], visit_vec[i]);
			score = loglikeli;
			break;
		}

		// Calculate a rank
		rank = 1;
		same_score_num = 0;
		for (j = 0; j < muser_num; j++) {
			if (i == j) continue;
			// Calculate a score between users i and j
			switch (reid_method) {
			case 1:	// transition matrix
				loglikeli = CalcScoreTransMat(etrace[i], visit_vec[j], trans_mat[j]);
				score2 = loglikeli;
				break;
			case 2:	// visiting probability vector
				loglikeli = CalcScoreVisitVec(etrace[i], visit_vec[j]);
				score2 = loglikeli;
				break;
			}
			if (score2 > score) rank++;
			else if (score2 == score) same_score_num++;
		}
		fprintf(fp, "%d,%d,%f,%d,%d\n", i, i, score, rank, same_score_num);
//		printf("%d,%d,%f,%d,%d\n", i, i, score, rank, same_score_num);
	}
	fclose(fp);
	printf(" done.\n");

	// Free
	free(muser);
	delete[] visit_vec;
	delete[] trans_mat;
	delete[] etrace;

	return 0;
}
