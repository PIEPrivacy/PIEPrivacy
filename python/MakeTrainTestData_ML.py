#!/usr/bin/env python3
import numpy as np
import csv
import sys

################################# Parameters ##################################
#sys.argv = ["MakeTrainTestData_ML.py", "4.0"]

if len(sys.argv) < 2:
    print("Usage:",sys.argv[0],"[Threshold of ratings]")
    sys.exit(0)

# Threshold of ratings
RatThrStr = sys.argv[1]
RatThr = float(sys.argv[1])

# Movie file (input)
MovFile = "../data/ML/movies.csv"
# Rating file (input)
RatFile = "../data/ML/ratings.csv"
# User index file (output)
UserIndexFile = "../data/ML/userindex_" + RatThrStr + ".csv"
# Movie index file (output)
MovIndexFile = "../data/ML/movieindex_" + RatThrStr + ".csv"
# Testing trace file (output)
TestTraceFile = "../data/ML/testtraces_AL.csv"
# Minimum number of movies per user
MinNumMov = 10
#MinNumMov = 500

############################# Read a rating file ##############################
# [output1]: movie_dic ({movie_id: [movie_index, counts]})
# [output2]: user_dic ({user_id: [user_index, movie_num]})
# [output3]: ucount_dic ({user_id: counts})
# [output4]: trace_list ([user_id, movie_id, rating, unixtime])
def ReadRat():
    # Initialization
    movie_dic = {}
    ucount_dic= {}
    trace_list = []

    # Read a movie file --> movie_dic ({movie_id: [movie_index, counts]})
    f = open(MovFile, "r", encoding='UTF-8')
    reader = csv.reader(f)
    next(reader)
    movie_index = 0
    for lst in reader:
        movie_id = lst[0]
        movie_dic[movie_id] = [movie_index, 0]
        movie_index += 1
    f.close()
    print("#Movies =", len(movie_dic))

    # Read a user count dictionary --> ucount_dic ({user_id: counts})
    f = open(RatFile, "r", encoding='UTF-8')
    reader = csv.reader(f)
    next(reader)
    for lst in reader:
        user_id = int(lst[0])
#        movie_id = lst[1]
        rating = float(lst[2])
#        ut = float(lst[3])
        # Update ucount_dic if the rating is >= RatThr
        if rating >= RatThr:
            ucount_dic[user_id] = ucount_dic.get(user_id, 0) + 1
    f.close()

    # A dictionary of users whose number of movies is >= MinNumMov --> user_dic ({user_id: [user_index, movie_num]})
    user_dic = {}
    for user_id, movie_num in sorted(ucount_dic.items()):
        # Continue if the number of movies for the user is below MinNumMov
        if movie_num < MinNumMov:
            continue
        user_dic[user_id] = [len(user_dic), movie_num]

    print("#Users =", len(user_dic))

    # Read a trace file --> trace_list ([user_id, movie_id, rating, unixtime])
    f = open(RatFile, "r")
    reader = csv.reader(f)
    next(reader)
    for lst in reader:
        user_id = int(lst[0])
        movie_id = lst[1]
        rating = float(lst[2])
        ut = float(lst[3])
        # Add an event to trace_list if user_id is in user_dic and the rating is >= RatThr
        if (user_id in user_dic) and (rating >= RatThr):
            trace_list.append([user_id, movie_id, rating, ut])
            movie_dic[movie_id][1] += 1
    f.close()

    # Sort trace_list in ascending order of (user_id, unixtime)
    trace_list.sort(key=lambda tup: (tup[0], tup[3]), reverse=False)

    return movie_dic, user_dic, ucount_dic, trace_list

#################################### Main #####################################
# Fix a seed
np.random.seed(1)

# Read the rating file
movie_dic, user_dic, ucount_dic, trace_list = ReadRat()

# Output user index
f = open(UserIndexFile, "w")
print("user_id,user_index,movie_num", file=f)
writer = csv.writer(f, lineterminator="\n")
for user_id in user_dic:
    lst = [user_id, user_dic[user_id][0], user_dic[user_id][1]]
    writer.writerow(lst)
f.close()

# Output movie index
f = open(MovIndexFile, "w")
print("movie_id,movie_index,counts", file=f)
writer = csv.writer(f, lineterminator="\n")
for movie_id in movie_dic:
    lst = [movie_id, movie_dic[movie_id][0], movie_dic[movie_id][1]]
    writer.writerow(lst)
f.close()

# Output testing traces
g = open(TestTraceFile, "w")
print("user_index,movie_index,rating,unixtime", file=g)
writer2 = csv.writer(g, lineterminator="\n")
ucount_dic_tmp = {}
for event in trace_list:
    user_id = event[0]
    movie_id = event[1]
    rating = event[2]
    ut = event[3]
    lst = [user_dic[user_id][0], movie_dic[movie_id][0], rating, ut]
    writer2.writerow(lst)
g.close()
