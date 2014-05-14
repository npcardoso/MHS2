import os
from os.path import join

env = Environment()

## scons output colorizer
try:
    from colorizer import colorizer
#    col = colorizer()
except:
    pass


root = Dir('#').abspath

vars = Variables('.scons.conf')

vars.Add('PATH', '', '')
vars.Add('LIBPATH', '', '')
vars.Add('CPPPATH', '', '')

vars.Add('prefix', '', join(root, 'obj', 'install'))
vars.Add('build_dir', '', join(root, 'obj'))
vars.Add('libext', '', 'so')

vars.Add('debug', '', False)

vars.Add('CXX', '', 'clang++')
vars.Update(env)


#Install/Build Dirs
for i in ['prefix', 'build_dir']:
    env[i]  = env.Dir(env[i]).abspath

#Build Flags

if(env['debug']):
    env['CCFLAGS'] = "-g -O0"
else:
    env['CCFLAGS'] = "-O3 -DNDEBUG"
env['CCFLAGS'] += " -std=c++11  -Wall -pedantic"

env['CPPPATH'] += root

env['ENV']['TERM'] = os.environ['TERM']


Export('env')
SConscript('SConscript', variant_dir=env['build_dir'])
