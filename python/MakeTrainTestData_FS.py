#!/usr/bin/env python3
import numpy as np
import csv
import sys

################################# Parameters ##################################
#sys.argv = ["MakeTrainTestData_FS.py", "AL"]
#sys.argv = ["MakeTrainTestData_FS.py", "IS"]
#sys.argv = ["MakeTrainTestData_FS.py", "NY"]
#sys.argv = ["MakeTrainTestData_FS.py", "TK"]
#sys.argv = ["MakeTrainTestData_FS.py", "JK"]
#sys.argv = ["MakeTrainTestData_FS.py", "KL"]
#sys.argv = ["MakeTrainTestData_FS.py", "SP"]

if len(sys.argv) < 2:
    print("Usage:",sys.argv[0],"[City]")
    sys.exit(0)

# City
City = sys.argv[1]

# POI file (input)
POIFile = "../data/FS/POI_XX.csv"
# Trace file (input)
TraceFile = "../data/FS/traces_XX.csv"
# User index file (output)
UserIndexFile = "../data/FS/userindex_XX.csv"
# POI index file (output)
POIIndexFile = "../data/FS/POIindex_XX.csv"
# Training trace file (output)
TrainTraceFile = "../data/FS/traintraces_XX.csv"
# Testing trace file (output)
TestTraceFile = "../data/FS/testtraces_XX.csv"
# Minimum time interval between two events (sec)
#MinTimInt = 1800
MinTimInt = 0
# Minimum number of locations per user
#MinNumLoc = 200
MinNumLoc = 10
# Number of extracted POIs
#ExtrPOINum = 1000
#ExtrPOINum = 2000

# Ratio of training locations
TrainRatio = 0.5
#TrainRatio = 1

########################### Read POI & trace files ############################
# [output1]: poi_dic ({poi_id: [poi_index, y, x, category, counts]})
# [output2]: user_dic ({user_id: [user_index, loc_num]})
# [output3]: ucount_dic ({user_id: counts})
# [output4]: trace_list ([user_id, poi_id, unixtime, dow, hour, min])
def ReadPOITrace():
    # Initialization
    poi_all_dic = {}
    poi_dic = {}
    poi_all_list = []
    ucount_dic= {}
    trace_list = []

    # Read a POI file --> poi_all_list ([poi_id, y, x, category, counts]})
    f = open(POIFile, "r")
    reader = csv.reader(f)
    next(reader)
    for lst in reader:
        poi_all_list.append([lst[0], lst[1], lst[2], lst[3], int(lst[4])])
    f.close()
    print("#POIs within the area of interest =", len(poi_all_list))

    # Sort poi_dic_all in descending order of counts
    poi_all_list.sort(key=lambda tup: tup[4], reverse=True)

#    # Extract POIs --> poi_dic ({poi_id: [poi_index, y, x, category, counts]})
#    for i in range(ExtrPOINum):
#        lst = poi_all_list[i]
#        poi_dic[lst[0]] = [len(poi_dic), lst[1], lst[2], lst[3], int(lst[4])]
#    print("#Extracted POIs =", len(poi_dic))

    # Read a user count dictionary --> ucount_dic ({user_id: counts})
    f = open(TraceFile, "r")
    reader = csv.reader(f)
    next(reader)
    utime_dic = {}
    for lst in reader:
#        if lst[1] not in poi_dic:
#            continue
        user_id = int(lst[0])
        poi_id = lst[1]
        ut = float(lst[4])
        dow = lst[8]
        ho = int(lst[9])
        mi = int(lst[10])
        # Update ucount_dic if the time interval is >= MinTimInt
        if user_id not in utime_dic or ut - utime_dic[user_id] >= MinTimInt:
            ucount_dic[user_id] = ucount_dic.get(user_id, 0) + 1
            utime_dic[user_id] = ut
    f.close()

    # A dictionary of users whose number of locations is >= MinNumLoc --> user_dic ({user_id: [user_index, loc_num]})
    user_dic = {}
    for user_id, loc_num in sorted(ucount_dic.items()):
        # Continue if the number of locations for the user is below MinNumLoc
        if loc_num < MinNumLoc:
            continue
        user_dic[user_id] = [len(user_dic), loc_num]

    print("#Users =", len(user_dic))

    # Read a POI file --> poi_all_dic ({poi_id: [y, x, category]})
    f = open(POIFile, "r")
    reader = csv.reader(f)
    next(reader)
    for lst in reader:
        poi_all_dic[lst[0]] = [lst[1], lst[2], lst[3]]
    f.close()

    # Read a trace file --> trace_list ([user_id, poi_id, unixtime, dow, hour, min])
    f = open(TraceFile, "r")
    reader = csv.reader(f)
    next(reader)
    utime_dic = {}
    for lst in reader:
#        if lst[1] not in poi_dic:
#            continue
        user_id = int(lst[0])
        poi_id = lst[1]
        ut = float(lst[4])
        dow = lst[8]
        ho = int(lst[9])
        mi = int(lst[10])
        # Add an event to trace_list if user_id is in user_dic and the time interval is >= MinTimInt
        if (user_id in user_dic) and (user_id not in utime_dic or ut - utime_dic[user_id] >= MinTimInt):
            trace_list.append([user_id, poi_id, ut, dow, ho, mi])
            utime_dic[user_id] = ut
    f.close()

    # Sort trace_list in ascending order of (user_id, unixtime)
    trace_list.sort(key=lambda tup: (tup[0], tup[2]), reverse=False)

    # Extract POIs --> poi_dic ({poi_id: [poi_index, y, x, category, counts]})
    for event in trace_list:
        user_id = int(event[0])
        poi_id = event[1]
        if poi_id not in poi_dic:
            poi_dic[poi_id] = [len(poi_dic), poi_all_dic[poi_id][0], poi_all_dic[poi_id][1], poi_all_dic[poi_id][2], 0]
        else:
            poi_index = poi_dic[poi_id][0]
            y = poi_dic[poi_id][1]
            x = poi_dic[poi_id][2]
            category = poi_dic[poi_id][3]
            cou = poi_dic[poi_id][4]
            poi_dic[poi_id] = [poi_index, y, x, category, cou+1]
    print("#POIs =", len(poi_dic))

    return poi_dic, user_dic, ucount_dic, trace_list

#################################### Main #####################################
# Fix a seed
np.random.seed(1)

# Replace XX with City
POIFile = POIFile.replace("XX", City)
TraceFile = TraceFile.replace("XX", City)
UserIndexFile = UserIndexFile.replace("XX", City)
POIIndexFile = POIIndexFile.replace("XX", City)
TrainTraceFile = TrainTraceFile.replace("XX", City)
TestTraceFile = TestTraceFile.replace("XX", City)

# Read POI & trace files
poi_dic, user_dic, ucount_dic, trace_list = ReadPOITrace()

# Output user index
f = open(UserIndexFile, "w")
print("user_id,user_index,loc_num", file=f)
writer = csv.writer(f, lineterminator="\n")
for user_id in user_dic:
    lst = [user_id, user_dic[user_id][0], user_dic[user_id][1]]
    writer.writerow(lst)
f.close()

# Output POI index
f = open(POIIndexFile, "w")
print("poi_id,poi_index,y,x,category,counts", file=f)
writer = csv.writer(f, lineterminator="\n")
for poi_id in poi_dic:
    lst = [poi_id, poi_dic[poi_id][0], poi_dic[poi_id][1], poi_dic[poi_id][2], poi_dic[poi_id][3], poi_dic[poi_id][4]]
    writer.writerow(lst)
f.close()

# Output training and testing traces
f = open(TrainTraceFile, "w")
print("user_index,poi_index,y,x,category,unixtime,dow,hour,min", file=f)
writer = csv.writer(f, lineterminator="\n")
g = open(TestTraceFile, "w")
print("user_index,poi_index,y,x,category,unixtime,dow,hour,min", file=g)
writer2 = csv.writer(g, lineterminator="\n")
ucount_dic_tmp = {}
for event in trace_list:
    user_id = event[0]
    poi_id = event[1]
    ucount_dic_tmp[user_id] = ucount_dic_tmp.get(user_id, 0)
#    ucount_dic_tmp[user_id] = ucount_dic_tmp.get(user_id, 0) + 1
    lst = [user_dic[user_id][0],poi_dic[poi_id][0],poi_dic[poi_id][1],poi_dic[poi_id][2],poi_dic[poi_id][3],event[2],event[3],event[4],event[5]]
    # Output a training trace
    if ucount_dic_tmp[user_id] < ucount_dic[user_id] * TrainRatio:
#    if ucount_dic_tmp[user_id] <= ucount_dic[user_id] * TrainRatio:
        writer.writerow(lst)
    # Output a testing trace
    else:
        writer2.writerow(lst)
    ucount_dic_tmp[user_id] += 1
f.close()
g.close()
