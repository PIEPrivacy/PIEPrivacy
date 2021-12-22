# PIEPrivacy
This is a source code of PIE (Personal Information Entropy) privacy in the following paper: 

Takao Murakami, Kenta Takahashi, "Toward Evaluating Re-identification Risks in the Local Privacy Model," Transactions on Data Privacy, Vol.14, Issue 3, pp.79-116, 2021. [[PDF](http://www.tdp.cat/issues21/tdp.a423a21.pdf)]

The source code is mostly implemented with C/C++. Data preprocessing is implemented with Python. This software is released under the MIT License.

# Directory Structure
- cpp/			&emsp;C/C++ codes.
- data/			&emsp;Output data (obtained by running codes).
  - FS/			&emsp;Output data in FS (Foursquare dataset).
  - ML/			&emsp;Output data in ML (MovieLens dataset).
- python/		&emsp;Python codes.
- results/		&emsp;Experimental results.
- LICENSE.txt		&emsp;MIT license.
- README.md		&emsp;This file.

# Usage

Below we describe how to build and run our code.

**(1) Install**

Install our code (C/C++) as follows.
```
$ cd cpp/
$ make
$ cd ../
```

**(2) Download and preprocess FS**

Download the [Foursquare dataset (Global-scale Check-in Dataset with User Social Networks)](https://sites.google.com/site/yangdingqi/home/foursquare-dataset) and place the dataset in data/FS/.

Run the following command to fix garbled text.

```
$ cd data/FS/
$ cat raw_POIs.txt | sed -e "s/Caf[^.]*\t/Caf\t/" > raw_POIs_fix.txt
$ cd ../
```

Run the following commands.

```
$ cd python/
$ python3 Read_FS.py ../data/FS/ AL
```

Then a POI file (POI_AL.csv) and trace file (traces_AL.csv) are output in data/FS/.

Run the following commands.

```
$ python3 MakeTrainTestData_FS.py AL
$ cd ../
```

Then a user index file (userindex_AL.csv), POI index file (POIindex_AL.csv), training trace file (traintraces_AL.csv), and testing trace file (testtraces_AL.csv) are output in data/FS/. Note that the data size is very large (larger than 10GB in total).

**(3) Download and preprocess ML**

Download the [MovieLens Latest dataset](https://grouplens.org/datasets/movielens/latest/) and place the dataset in data/ML/.

Run the following commands.

```
$ cd python/
$ python3 MakeTrainTestData_ML.py 4.0
$ cd ../
```

Then a user index file (userindex_4.0.csv), movie index file (movieindex_4.0.csv), and testing trace file (testtraces_AL.csv) are output in data/FS/.

**(4) Evaluating PSE (Personal Identification System Entropy) in FS**

Run the following commands.

```
$ cd cpp/
$ ./RR_FS ../data/FS AL -1 [Testing Trace Length (1-5)]
```

Then a testing trace without no obfuscation (testtraces_AL_RR-e-1-l[1-5].csv) is output in data/FS/. To use RR with privacy budget epsilon, change "-1" in the 4th argument to epsilon.

Run the following commands.

```
$ ./CalcScore_FS ../data/FS AL org 2 1
$ ./CalcScore_FS ../data/FS AL RR-e-1-l[1-5] 2 1
```

Then genuine scores (ge_score_AL_RR-e-1-l1_ak2_rm1_rnl1_nl0.csv) and impostor scores (im_score_AL_RR-e-1-l1_ak2_rm1_rnl1_nl0_r1.csv) are output in data/FS/.

Run the following commands.

```
$ ./EvalKL ../data/FS AL_ak2_rm1_rnl1_nl0 1 100000 0
$ ./EvalKL ../data/FS AL_RR-e-1-l[1-5]_ak2_rm1_rnl1_nl0 1 0 0
```

Then the PSE (kl_AL_RR-e-1-l1_ak2_rm1_rnl1_nl0_r1_es0.csv) is output in data/FS/. This file contains the KL divergence calculated by the generalized k-NN estimator.

We plotted Figure 5 in our paper based on this result. See results/Fig5_PSEmax.xlsx for details.

**(5) Evaluating PSE (Personal Identification System Entropy) in ML**

Change FS to ML in (4).

# Execution Environment
We used CentOS 7.5 with gcc 4.8.5 and python 3.6.5.
