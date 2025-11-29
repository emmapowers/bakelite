
gen: proto.h proto.py

proto.h: proto.bakelite bakelite.h
	bakelite gen -l cpptiny -i proto.bakelite -o proto.h

proto.py: proto.bakelite bakelite_runtime
	bakelite gen -l python -i proto.bakelite -o proto.py

bakelite.h:
	bakelite runtime -l cpptiny -o bakelite.h

bakelite_runtime:
	bakelite runtime -l python

clean:
	rm -rf proto.h proto.py bakelite.h bakelite_runtime
