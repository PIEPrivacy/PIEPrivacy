#include <iostream>
#include <map>
#include <tuple>
#include <stdio.h>
#include <string.h>
#include <stdlib.h>
#include <math.h>
#include "mt19937ar2.h"

using namespace std;

FILE *FileOpen(const char *filename, const char *mode) {
	FILE *fp;

	if ((fp = fopen(filename, mode)) == NULL) {
		printf("cannot open %s\n", filename);
		exit(-1);
	}
	return fp;
}

// Read POIs
void ReadPOIs(char *ttrace_file, char *etrace_file, map<int, int> &user_exist, map<int, int> &poi_exist){
	int user_index, poi_index;
	char s[1025];
	FILE *fp;
	char *tok;

	// Open the training trace file
	fp = FileOpen(ttrace_file, "r");
	// Skip the header
	fgets(s, 1024, fp);
	// Read POIs from the training trace file --> poi_exist
	while (fgets(s, 1024, fp) != NULL) {
		tok = strtok(s, ",");
		user_index = atoi(tok);
		user_exist[user_index] = 1;

		tok = strtok(NULL, ",");
		poi_index = atoi(tok);
		poi_exist[poi_index] = 1;
	}
	fclose(fp);

	// Open the testing trace file
	fp = FileOpen(etrace_file, "r");
	// Skip the header
	fgets(s, 1024, fp);
	// Read POIs from the testing trace file --> poi_exist
	while (fgets(s, 1024, fp) != NULL) {
		tok = strtok(s, ",");
		user_index = atoi(tok);
		user_exist[user_index] = 1;

		tok = strtok(NULL, ",");
		poi_index = atoi(tok);
		poi_exist[poi_index] = 1;
	}
	fclose(fp);
}

// Obfuscate testing traces via GLH
void ObfTraces(char *etrace_file, char *obftrace_file, char *obftrace_h_file, char *obftrace_c_file, 
				int poi_num, double eps, int m, int g, int trace_len){
	double eps_per_loc;
	int user_index, user_index_prev;
	int poi_index;
	int obf_code_index;
	int time_ins;
	double rnd;
	double beta;
	char s[1025];
	FILE *fp,*fp2,*fp3,*fp4;
	char *tok;

	map<int, int> user2hash;
	map<int, int>::iterator hitr;
	map<tuple<int, int>, int> hashpoi2code;
	map<tuple<int, int>, int>::iterator citr;
	int hindex, cindex;
	int i;

	// Probability of sending the true code in RR --> beta
//	eps_per_loc = eps / (double)trace_len;
	eps_per_loc = eps;

	if (eps_per_loc == -1) beta = 1.0;
	else beta = (exp(eps_per_loc) - 1) / (exp(eps_per_loc) + g - 1);
	printf("[beta]: %e\n", beta);

	// Open the testing trace file
	fp = FileOpen(etrace_file, "r");
	// Skip the header
	fgets(s, 1024, fp);

	// Open the obfuscated trace file
	fp2 = FileOpen(obftrace_file, "w");
	// Write the header
	fprintf(fp2, "user_index,obf_code_index,org_code_index\n");

	// if m = 1, then generate a code for each POI
	if(m == 1){
		for(i = 0; i < poi_num; i++){
			cindex = genrand_int32() % g;
			hashpoi2code[make_tuple(0, i)] = cindex;
		}
	}

	// Obfuscate testing traces
	time_ins = 0;
	user_index_prev = -1;
	while (fgets(s, 1024, fp) != NULL) {
		strtok(s, ",");
		user_index = atoi(s);
		tok = strtok(NULL, ",");
		poi_index = atoi(tok);

		if (user_index_prev != -1 && user_index != user_index_prev) {
			time_ins = 0;
		}

		// Continue if the trace length reaches trace_len
		if(trace_len != 0 && time_ins >= trace_len) continue;

		// Randomly generate a hash index for a new user --> user2hash
		if(user2hash.count(user_index) == 0){
			hindex = genrand_int32() % m;
			user2hash[user_index] = hindex;
		}
		// Existing hash index --> hindex
		else{
			hindex = user2hash[user_index];
		}

		// Randomly generate a code index for a new (hash,poi) pair --> hashpoi2code
		if(hashpoi2code.count(make_tuple(hindex, poi_index)) == 0){
			cindex = genrand_int32() % g;
			hashpoi2code[make_tuple(hindex, poi_index)] = cindex;
		}
		// Existing code index --> cindex
		else{
			cindex = hashpoi2code[make_tuple(hindex, poi_index)];
		}

		// RR --> obf_code_index
		rnd = genrand_real1();
		obf_code_index = genrand_int32() % g;	// Randomly generate a code with probability 1 - beta
		if(rnd <= beta) obf_code_index = cindex;	// Send the true code with probability beta

		// Write the obfuscated code index
		fprintf(fp2, "%d,%d,%d\n", user_index, obf_code_index, cindex);

		user_index_prev = user_index;
		time_ins++;
	}

	fclose(fp);
	fclose(fp2);

	// Open the hash file
	fp3 = FileOpen(obftrace_h_file, "w");
	// Write the header
	fprintf(fp3, "user_index,hash_index\n");
	// Write the hash index for each user
	for (hitr = user2hash.begin(); hitr != user2hash.end(); hitr++) {
		fprintf(fp3, "%d,%d\n", hitr->first, hitr->second);
	}
	fclose(fp3);

	// Open the code file
	fp4 = FileOpen(obftrace_c_file, "w");
	// Write the header
	fprintf(fp4, "hash_index,poi_index,code_index\n");
	// Write the code index for each (hash,poi) pair
	for (citr = hashpoi2code.begin(); citr != hashpoi2code.end(); citr++) {
		fprintf(fp4, "%d,%d,%d\n", get<0>(citr->first), get<1>(citr->first), citr->second);
	}
	fclose(fp4);
}

int main(int argc, char *argv[])
{
	char outdir[129], city[129], ttrace_file[129], etrace_file[129];
	char obftrace_file[129], obftrace_h_file[129], obftrace_c_file[129];
	char eps_s[129];
	double eps;
	int trace_len;

	map<int, int> poi_exist;
	int *poi;
	map<int, int>::iterator pitr;
	int poi_num;

	int m, g;
	map<int, int> user_exist;
	int *user;
	map<int, int>::iterator uitr;
	int user_num;

	int i, j;
	FILE *fp;

	// Initialization of Mersennne Twister
	unsigned long init[4] = { 0x123, 0x234, 0x345, 0x456 }, length = 4;
	init_by_array(init, length);

	if (argc < 3) {
		printf("Usage: %s [DataDir] [City] ([Epsilon (default:1)] [m (default: 0)] [g (default: 2)] [TraceLen (default:1)])\n\n", argv[0]);
		printf("[DataDir]: Data directory\n");
		printf("[City]: City (AL/IS/JK/KL/NY/SP/TK)\n");
		printf("[Epsilon]: Epsilon for each location (-1: no noise)\n");
		printf("[m]: Number of hashes (0: determine m based on [Bassily+, STOC15] with beta = 0.05)\n");
		printf("[g]: Category size after encoding\n");
		printf("[TraceLen]: Trace length (number of testing locations) for each user (0: all)\n\n");
		return -1;
	}

	sprintf(outdir, argv[1]);
	sprintf(city, argv[2]);
	sprintf(eps_s, "1");
	if (argc >= 4){
		sprintf(eps_s, argv[3]);
	}
	eps = atof(eps_s);
	m = 0;
	if (argc >= 5) m = atoi(argv[4]);
	g = 2;
	if (argc >= 6) g = atoi(argv[5]);
	trace_len = 1;
	if (argc >= 7) trace_len = atoi(argv[6]);
	printf("%s,%s,%s,%f,%d,%d,%d\n", outdir, city, eps_s, eps, m, g, trace_len);

	sprintf(ttrace_file, "%s/traintraces_%s.csv", outdir, city);
	sprintf(etrace_file, "%s/testtraces_%s.csv", outdir, city);
	sprintf(obftrace_file, "%s/testtraces_%s_GLH-e%s-m%d-g%d-l%d.csv", outdir, city, eps_s, m, g, trace_len);
	sprintf(obftrace_h_file, "%s/testtraces_%s_GLH-e%s-m%d-g%d-l%d_h.csv", outdir, city, eps_s, m, g, trace_len);
	sprintf(obftrace_c_file, "%s/testtraces_%s_GLH-e%s-m%d-g%d-l%d_c.csv", outdir, city, eps_s, m, g, trace_len);

	printf("[Training]: %s\n", ttrace_file);
	printf("[Testing]: %s\n", etrace_file);
	printf("[Obfuscated]: %s\n", obftrace_file);
	printf("[Hash]: %s\n", obftrace_h_file);
	printf("[Code]: %s\n", obftrace_c_file);

	// Read POIs --> user_exist, poi_exist
	ReadPOIs(ttrace_file, etrace_file, user_exist, poi_exist);

	// Number of users --> user_num
	user_num = 0;
	for (uitr = user_exist.begin(); uitr != user_exist.end(); uitr++) {
		if (uitr->second == 1){
//			if (user_num % 100000 == 0) cout << uitr->first << " " << uitr->second << endl;
			user_num++;
		}
	}
	printf("[#Users]: %d\n", user_num);

	// Number of POIs --> poi_num
	poi_num = 0;
	for (pitr = poi_exist.begin(); pitr != poi_exist.end(); pitr++) {
		if (pitr->second == 1){
//			if (poi_num % 100000 == 0) cout << pitr->first << " " << pitr->second << endl;
			poi_num++;
		}
	}
	printf("[#POIs]: %d\n", poi_num);

	// POIs --> poi
	poi = new int[poi_num];
	i = 0;
	for (pitr = poi_exist.begin(); pitr != poi_exist.end(); pitr++) {
		poi[i++] = pitr->first;
	}

	// Confirm that every POI exists in training and testing traces
	for(i=0;i<poi_num;i++){
		if(poi[i] != i){
			printf("Some POIs are missing in training and testing traces.\n");
			exit(-1);
		}
//		if(i % 100000 == 0 || i == poi_num - 1) cout << i << " " << poi[i] << endl;
	}

	// Determine m based on [Bassily+, STOC15] with beta = 0.05
	if(m == 0){
		m = (int)(eps * eps * user_num * log(poi_num+1) * log(2.0/0.05) / log(2*poi_num/0.05));
		if(m < 0){
			printf("#Hashes becomes negative, so assign MAX_INT (2147483647).\n");
			m = 2147483647;
		}
	}
	printf("[#Hashes]: %d\n", m);
	printf("[#Categories]: %d\n", g);

	// Obfuscate testing traces via GLH
	ObfTraces(etrace_file, obftrace_file, obftrace_h_file, obftrace_c_file, poi_num, eps, m, g, trace_len);

	delete[] poi;

	return 0;
}