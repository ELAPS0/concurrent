project('concurrent','cpp')
header = include_directories('hash-library')
thread_dep = dependency('threads')
src=['concurrent.cpp', 'hash-library/keccak.cpp']
executable('cncrrnt', sources : src, include_directories : header, dependencies : thread_dep)
