"""Prototype: the strongly connected components of the directory-part include graph, at three
minimum edge weights - the layering the code actually has, before any intent is applied."""
import os, sys
sys.path.insert(0, os.path.join(os.path.dirname(os.path.abspath(__file__)), '..', '..', '..', '..', 'cmake'))
import pygplates_source_closure as psc
closure = psc.Closure(psc.discover_roots()); reached=set(closure.files())
files=[f for f in psc.all_source_files() if '/' in f and psc.top_dir(f)!='unit-test']; fs=set(files)
def node(r):
    d=psc.top_dir(r); return d if r in reached or d in psc.FORBIDDEN_DIRS else d+'+'
res=psc.Closure.__new__(psc.Closure); res._resolve_cache={}
w={}
for r in files:
    for i in psc._QUOTED_INCLUDE_RE.findall(psc.read_stripped(os.path.join(psc.SRC_DIR,r))):
        t=res._resolve(i,os.path.dirname(r))
        if t and t in fs and node(r)!=node(t): w[(node(r),node(t))]=w.get((node(r),node(t)),0)+1
nodes=sorted({n for e in w for n in e})
def sccs(edges):
    adj={n:[] for n in nodes}
    for a,b in edges: adj[a].append(b)
    idx={};low={};st=[];on=set();out=[];c=[0]
    sys.setrecursionlimit(10000)
    def v(n):
        idx[n]=low[n]=c[0];c[0]+=1;st.append(n);on.add(n)
        for m in adj[n]:
            if m not in idx: v(m);low[n]=min(low[n],low[m])
            elif m in on: low[n]=min(low[n],idx[m])
        if low[n]==idx[n]:
            g=[]
            while True:
                m=st.pop();on.discard(m);g.append(m)
                if m==n:break
            out.append(sorted(g))
    for n in nodes:
        if n not in idx: v(n)
    return out
for th in (1,3,10):
    print('min weight',th,[g for g in sccs([e for e in w if w[e]>=th]) if len(g)>1])
