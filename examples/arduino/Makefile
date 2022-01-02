
gen: proto.h proto.py

proto.h: proto.bakelite bakelite.h
	bakelite gen -l cpptiny -i proto.bakelite -o proto.h

proto.py: proto.bakelite
	bakelite gen -l python -i proto.bakelite -o proto.py

bakelite.h:
	bakelite runtime -l cpptiny -o bakelite.h
