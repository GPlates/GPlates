"""Prototype: a filtered Mermaid layer diagram from the pygplates_source_closure.py include data.

Nodes are directories, split into a module part and a GPlates-only part where the pyGPlates
closure takes only some of a directory. Downward edges are transitively reduced; edges that go
UP the intended layering are all kept and drawn red.
"""
import os
import sys

sys.path.insert(0, os.path.join(os.path.dirname(os.path.abspath(__file__)), '..', '..', '..', '..', 'cmake'))
import pygplates_source_closure as psc

# Intended layers, lowest first. A node name is '<dir>' for the part of a directory that is in
# the pyGPlates module and '<dir>+' for its GPlates-only part. Nodes in one layer are peers.
LAYERS = [
	('Foundation', ['global', 'utils']),
	('Maths + serialisation', ['maths', 'scribe']),
	('Model', ['model', 'property-values']),
	('Shared core', ['file-io', 'app-logic', 'gui']),
	('pyGPlates bindings', ['api']),
	('GPlates engine', ['app-logic+', 'file-io+', 'scribe+', 'opengl', 'data-mining', 'maths+',
		'property-values+', 'global+', 'utils+']),
	('GPlates user interface', ['gui+', 'presentation', 'view-operations', 'canvas-tools', 'api+',
		'cli', 'qt-widgets']),
]
# Directories whose upward includes are accepted: 'utils' is cross-cutting by design.
CROSS_CUTTING = {'utils'}
SKIP_DIRS = {'unit-test'}


def main():
	closure = psc.Closure(psc.discover_roots())
	reached = set(closure.files())
	files = [f for f in psc.all_source_files() if '/' in f and psc.top_dir(f) not in SKIP_DIRS]
	file_set = set(files)

	def node_of(rel):
		d = psc.top_dir(rel)
		return d if rel in reached or d in psc.FORBIDDEN_DIRS else d + '+'

	resolver = psc.Closure.__new__(psc.Closure)
	resolver._resolve_cache = {}
	weight = {}
	sizes = {}
	for rel in files:
		sizes[node_of(rel)] = sizes.get(node_of(rel), 0) + 1
		for inc in psc._QUOTED_INCLUDE_RE.findall(psc.read_stripped(os.path.join(psc.SRC_DIR, rel))):
			res = resolver._resolve(inc, os.path.dirname(rel))
			if res is None or res not in file_set:
				continue
			u, v = node_of(rel), node_of(res)
			if u != v:
				weight[(u, v)] = weight.get((u, v), 0) + 1

	layer_of = {}
	for i, (_, nodes) in enumerate(LAYERS):
		for n in nodes:
			layer_of[n] = i
	nodes = set(sizes)
	module_nodes = {n for n in nodes if not n.endswith('+') and n not in psc.FORBIDDEN_DIRS}
	only_module = '--pygplates' in sys.argv
	if only_module:
		nodes = module_nodes
		weight = {e: n for e, n in weight.items() if e[0] in nodes and e[1] in nodes}
	missing = nodes - set(layer_of)
	if missing:
		sys.exit('unplaced nodes: %s' % sorted(missing))

	down = {e for e in weight if layer_of[e[0]] > layer_of[e[1]]}
	peer = {e for e in weight if layer_of[e[0]] == layer_of[e[1]]}
	up_all = {e for e in weight if layer_of[e[0]] < layer_of[e[1]]}
	allowed = {e for e in up_all if e[0].rstrip('+') in CROSS_CUTTING}
	up = up_all - allowed

	# Transitive reduction of downward edges, weakest first (peer edges are not drawn, so they
	# must not count as paths).
	kept = set(down)

	def reachable(src, dst, edges):
		adj = {}
		for a, b in edges:
			adj.setdefault(a, []).append(b)
		seen, stack = {src}, [src]
		while stack:
			for b in adj.get(stack.pop(), []):
				if b == dst:
					return True
				if b not in seen:
					seen.add(b)
					stack.append(b)
		return False

	for e in sorted(down, key=lambda e: (weight[e], e)):
		if reachable(e[0], e[1], kept - {e}):
			kept.discard(e)
	down_kept = kept & down

	def nid(n):
		return n.replace('-', '_').replace('+', '_gp')

	def label(n):
		d = n.rstrip('+')
		total = sum(v for k, v in sizes.items() if k.rstrip('+') == d)
		if n.endswith('+'):
			return '%s<br/><small>GPlates-only part, %d files</small>' % (d, sizes[n]) \
				if total != sizes[n] else '%s<br/><small>%d files</small>' % (d, sizes[n])
		return '%s<br/><small>%d files%s</small>' % (
			d, sizes[n], '' if total == sizes[n] else ' of %d' % total)

	out = ['flowchart BT']
	for i, (title, layer_nodes) in enumerate(LAYERS):
		present = [n for n in layer_nodes if n in nodes]
		if not present:
			continue
		out.append('  subgraph L%d ["%s"]' % (i, title))
		out.append('    direction LR')
		for n in present:
			out.append('    %s["%s"]:::%s' % (nid(n), label(n), 'mod' if n in module_nodes else 'gp'))
		out.append('  end')
	links = []
	for e in sorted(down_kept, key=lambda e: (layer_of[e[0]], e)):
		arrow = '==>' if weight[e] >= 100 else '-->'
		out.append('  %s %s %s' % (nid(e[0]), arrow, nid(e[1])))
		links.append('stroke:#888')
	for e in sorted(up, key=lambda e: -weight[e]):
		out.append('  %s -. "%d" .-> %s' % (nid(e[0]), weight[e], nid(e[1])))
		links.append('stroke:#d33,stroke-width:2px,color:#d33')
	for e in sorted(allowed, key=lambda e: -weight[e]):
		out.append('  %s -. "%d" .-> %s' % (nid(e[0]), weight[e], nid(e[1])))
		links.append('stroke:#b7791f,stroke-dasharray:4 4,color:#b7791f')
	out.append('  classDef mod fill:#dbeafe,stroke:#1d4ed8,color:#111')
	out.append('  classDef gp fill:#f3f4f6,stroke:#6b7280,color:#111')
	for style in sorted(set(links)):
		out.append('  linkStyle %s %s' % (','.join(str(i) for i, s in enumerate(links) if s == style), style))

	print('\n'.join(out))
	print('\n%% downward %d -> kept %d after reduction; peer %d (not drawn); upward %d'
		% (len(down), len(down_kept), len(peer), len(up)), file=sys.stderr)
	for e in sorted(up, key=lambda e: -weight[e]):
		print('%%  UP %-18s -> %-18s %4d' % (e[0], e[1], weight[e]), file=sys.stderr)


main()
