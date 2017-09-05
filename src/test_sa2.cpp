/**
* Copyright 2015 D-Wave Systems Inc.
*
* Licensed under the Apache License, Version 2.0 (the "License");
* you may not use this file except in compliance with the License.
* You may obtain a copy of the License at
*
*    http://www.apache.org/licenses/LICENSE-2.0
*
* Unless required by applicable law or agreed to in writing, software
* distributed under the License is distributed on an "AS IS" BASIS,
* WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
* See the License for the specific language governing permissions and
* limitations under the License.
*/

#include <vector>
#include <stdio.h>
#include <ctime>
#include <chrono>
#include <string>

#include "ch_annealer.h"

using namespace std;

int show_usage(int argc, char **argv) {
	fprintf(stdout, "Usage: %s <options>:\n", argv[0]);
	fprintf(stdout, "\t-h, --help\t\t\tShow this help message\n");
	fprintf(stdout, "\t-if, --input-file [FILENAME]\tInput problem file name, (must be provided)\n");
	fprintf(stdout, "\t-sa, --samples [SAMPLES]\tNumber of samples (default=24)\n");
	fprintf(stdout, "\t-sw, --sweeps [SWEEPS]\t\tNumber of sweeps (default=2000)\n");
	fprintf(stdout, "\t-v, --verbose [MODE]\t\tVerbose mode, print timings (mode=1), energy histogram (mode=2) or states (mode=4) (default: 3)\n");
	fprintf(stdout, "\t-s, --seed [SEED]\t\tSolver random seed (default: system time)\n");
	return 0;
}

int load_problem(
	const char *inputfile,
	vector<double> & fields,
	vector<int> & coupler_starts,
	vector<int> & coupler_ends,
	vector<double> & coupler_values)
{
	FILE *f = fopen(inputfile, "r");
	if (!f) return 1;

	int ret, n, cs, ce;
	double cv;
	char line[256] = { 0 };
	if (fgets(line, sizeof(line), f) == NULL) return -1;

	ret = sscanf(line, "%d\n", &n);
	if (ret != 1) {
		fclose(f);
		return -2;
	}

	fields.resize(n, 0);

	while (true) {
		if (fgets(line, sizeof(line), f) == NULL) break;
		ret = sscanf_s(line, "%d %d %lf\n", &cs, &ce, &cv);
		if (ret != 3) {
			fprintf(stderr, "invalid input file: %s\n", inputfile);
			fclose(f);
			return -4;
		}
		if (cs == ce) {
			fields[cs] = cv;
		}
		else {
			coupler_starts.push_back(cs);
			coupler_ends.push_back(ce);
			coupler_values.push_back(cv);
		}
	}

	fclose(f);

	return 0;
}

int main(int argc, char **argv) {

	string inputfilestr;

	const char* inputfile = NULL;
	char * inputchar = NULL;
	int samples = 8;
	int sweeps = 200;
	int sweeps_per_beta = 1;
	double beta_start = .01;
	double beta_end = 200.0;
	unsigned int seed = 0;        // default system time-based random seed
	int verbose = 7;

	chrono::time_point<chrono::system_clock> t0, t1, t2;
	chrono::duration<double> init_time, anneal_time, total_time;

	//int opt = 1;
	//while (opt < argc) {
	//	if (!strcmp(argv[opt], "-h") || !strcmp(argv[opt], "--help")) {
	//		return show_usage(argc, argv);
	//	}
	//	else if (!strcmp(argv[opt], "-v") || !strcmp(argv[opt], "--verbose")) {
	//		opt++;
	//		if (opt == argc) return show_usage(argc, argv);
	//		verbose = atoi(argv[opt]);
	//	}
	//	else if (!strcmp(argv[opt], "-if") || !strcmp(argv[opt], "--input-file")) {
	//		opt++;
	//		if (opt == argc) return show_usage(argc, argv);
	//		int success = sprintf(inputchar, "Qs_bern_txt\\Q_%i_%i", 45, 4);
	//		inputfile = (const char *)inputchar;
	//		//inputfile = "Qs_bern_txt\\Q_45_4.txt";
	//		//inputfile = argv[opt];
	//		fprintf(stderr, "input file %s\n", inputfile);
	//	}
	//	else if (!strcmp(argv[opt], "-sa") || !strcmp(argv[opt], "--samples")) {
	//		opt++;
	//		if (opt == argc) return show_usage(argc, argv);
	//		samples = atoi(argv[opt]);
	//	}
	//	else if (!strcmp(argv[opt], "-sw") || !strcmp(argv[opt], "--sweeps")) {
	//		opt++;
	//		if (opt == argc) return show_usage(argc, argv);
	//		sweeps = atoi(argv[opt]);
	//	}
	//	else if (!strcmp(argv[opt], "-s") || !strcmp(argv[opt], "--seed")) {
	//		opt++;
	//		if (opt == argc) return show_usage(argc, argv);
	//		seed = atoi(argv[opt]);
	//	}
	//	else {
	//		return show_usage(argc, argv);
	//	}
	//	opt++;
	//}

	//const char * inputfile = inputfilestr.c_str();

	//int su/*ccess = sprintf(inputchar, "Qs_bern_txt\\Q_%i_%i.txt", 45, 4);
	//inputchar = "Qs_bern_txt\\Q_45_4";
	//inpu*/tfile = (const char *)inputchar;
	//inputfile = "Qs_bern_txt\\Q_45_4.txt";
	//inputfile = argv[opt];
	//fprintf(stderr, "input file %s\n", inputfile);


	for (int i = 5; i < 50; ++i) {
		for (int j = 1; j <= 5; ++j) {

			string input_file = "Qs_bern_txt\\Q_" + std::to_string(i)
				+ "_" + std::to_string(j) + ".txt";
			fprintf(stdout, "input file:\t%s\n", input_file.c_str());

			//if (!inputfile) return show_usage(argc, argv);
			vector<int> coupler_starts, coupler_ends;
			vector<double> fields, coupler_values;

			int ret = load_problem(input_file.c_str(), fields, coupler_starts, coupler_ends, coupler_values);
			if (ret != 0) {
				fprintf(stderr, "error loading problem from file: %s", inputfile);
				return ret;
			}

			t0 = chrono::system_clock::now();

			/* init */
			chimera_annealer dwave_sa(fields, coupler_starts, coupler_ends, coupler_values, seed);

			t1 = chrono::system_clock::now();

			/* anneal */
			dwave_sa.anneal(samples, sweeps, sweeps_per_beta, beta_start, beta_end, false);

			t2 = chrono::system_clock::now();


			if (verbose & 4) dwave_sa.print_states();
			if (verbose & 2) {

				dwave_sa.print_energies(true);
				dwave_sa.print_energy_histogram();
			}


			init_time = t1 - t0;
			anneal_time = t2 - t1;
			total_time = t2 - t0;

			if (verbose & 1) {
				fprintf(stdout, "\n");
				fprintf(stdout, "init time:\t%16lf\n", init_time.count());
				fprintf(stdout, "anneal time:\t%16lf\n", anneal_time.count());
				fprintf(stdout, "total time:\t%16lf\n", total_time.count());
				fprintf(stdout, "\n\n");
			}
		}
	}

	return 0;
}

