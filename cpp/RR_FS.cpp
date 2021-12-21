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
void ReadPOIs(char *ttrace_file, char *etrace_file, map<int, int> &poi_exist){
	int poi_index;
	char s[1025];
	FILE *fp;
	char *tok;

	// Open the training trace file
	fp = FileOpen(ttrace_file, "r");
	// Skip the header
	fgets(s, 1024, fp);
	// Read POIs from the training trace file --> poi_exist
	while (fgets(s, 1024, fp) != NULL) {
		strtok(s, ",");
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
		strtok(s, ",");
		tok = strtok(NULL, ",");
		poi_index = atoi(tok);
		poi_exist[poi_index] = 1;
	}
	fclose(fp);
}

// Obfuscate testing traces via RR
void ObfTraces(char *etrace_file, char *obftrace_file, int poi_num, double eps, int trace_len){
	double eps_per_loc;
	int user_index, user_index_prev;
	int poi_index;
	int obf_poi_index;
	int time_ins;
	double rnd;
	double alpha;
	char s[1025];
	FILE *fp,*fp2;
	char *tok;

	// Probability of sending the true alphabet in RR --> alpha
//	eps_per_loc = eps / (double)trace_len;
	eps_per_loc = eps;

	if (eps_per_loc == -1) alpha = 1.0;
	else alpha = (exp(eps_per_loc) - 1) / (exp(eps_per_loc) + poi_num - 1);
	printf("[alpha]: %e\n", alpha);

	// Open the testing trace file
	fp = FileOpen(etrace_file, "r");
	// Skip the header
	fgets(s, 1024, fp);

	// Open the obfuscated trace file
	fp2 = FileOpen(obftrace_file, "w");
	// Write the header
	fprintf(fp2, "user_index,poi_index\n");

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

		// RR --> obf_poi_index
		rnd = genrand_real1();
		obf_poi_index = genrand_int32() % poi_num;	// Randomly generate a alphabet with probability 1 - alpha
		if(rnd <= alpha) obf_poi_index = poi_index;	// Send the true alphabet with probability alpha
//		else obf_poi_index = genrand_int32() % poi_num;	// Randomly generate a alphabet with probability 1 - alpha

		// Write the obfuscated location
		fprintf(fp2, "%d,%d\n", user_index, obf_poi_index);

		user_index_prev = user_index;
		time_ins++;
	}

	fclose(fp);
	fclose(fp2);
}

int main(int argc, char *argv[])
{
	char outdir[129], city[129], ttrace_file[129], etrace_file[129], obftrace_file[129];
	char eps_s[129];
	double eps;
	int trace_len;
	map<int, int> poi_exist;
	int *poi;
	map<int, int>::iterator pitr;
	int poi_num;
	int i, j;
	FILE *fp;

	// Initialization of Mersennne Twister
	unsigned long init[4] = { 0x123, 0x234, 0x345, 0x456 }, length = 4;
	init_by_array(init, length);

	if (argc < 3) {
		printf("Usage: %s [DataDir] [City] ([Epsilon (default:1)] [TraceLen (default:1)])\n\n", argv[0]);
		printf("[DataDir]: Data directory\n");
		printf("[City]: City (AL/IS/JK/KL/NY/SP/TK)\n");
		printf("[Epsilon]: Epsilon for each location (-1: no noise)\n");
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
	trace_len = 1;
	if (argc >= 5) trace_len = atoi(argv[4]);
//	printf("%s,%s,%s,%f,%d\n", outdir, city, eps_s, eps, trace_len);

	sprintf(ttrace_file, "%s/traintraces_%s.csv", outdir, city);
	sprintf(etrace_file, "%s/testtraces_%s.csv", outdir, city);
	sprintf(obftrace_file, "%s/testtraces_%s_RR-e%s-l%d.csv", outdir, city, eps_s, trace_len);

	printf("[Training]: %s\n", ttrace_file);
	printf("[Testing]: %s\n", etrace_file);
	printf("[Obfuscated]: %s\n", obftrace_file);

	// Read POIs --> poi_exist
	ReadPOIs(ttrace_file, etrace_file, poi_exist);
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

	// Obfuscate testing traces via RR
	ObfTraces(etrace_file, obftrace_file, poi_num, eps, trace_len);

	delete[] poi;

	return 0;
}