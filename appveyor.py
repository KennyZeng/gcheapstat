import os
import re
import sys
import zipfile
import subprocess

def zip_binary(zipname, platform, configuration):
  if platform == 'x86':
    name = 'gcheapstat32.exe'
  elif platform == 'x64':
    name = 'gcheapstat64.exe'
  else:
    sys.exit('Expected platform either x86 or x64')
  path = os.path.join("out", platform, configuration, 'gcheapstat.exe')
  zipf = zipfile.ZipFile(zipname, 'a', zipfile.ZIP_DEFLATED)
  zipf.write(path, name)
  zipf.close()
  return zipname

formatRe = re.compile(r'Format: RSDS, {([0-9a-fA-F\-]+)},\s*(\d+)')
def dumpbin(path):
  proc = subprocess.Popen([
    r'dumpbin.exe',
    path, '/headers'], stdout=subprocess.PIPE)
  for line in iter(proc.stdout.readline, ''):
    match = formatRe.search(line.decode('utf-8'))
    if match:
      groups = match.groups()
      guid = groups[0].replace('-', '')
      age = int(groups[1])
      return guid, age

def zip_pdb(zipf, path, name):
  guid, age = dumpbin(os.path.join(path, name + '.exe'))
  pdb_name = name + '.pdb'
  zipf.write(os.path.join(path, pdb_name), r'%s\%s%x\%s' % (pdb_name, guid, age, pdb_name))

def zip_symbols(zipname, platform, configuration):
  zipf = zipfile.ZipFile(zipname, 'a', zipfile.ZIP_DEFLATED)
  path = os.path.join('out', platform, configuration)
  zip_pdb(zipf, path, 'gcheapstat')
  zip_pdb(zipf, path, 'gcheapstatsvc')
  zipf.close()

def build(platform, configuration):
  subprocess.call(['msbuild',
    'gcheapstat.sln',
    '/p:Platform=' + platform,
    '/p:Configuration=' + configuration,
    '/l:C:\\Program Files\\AppVeyor\\BuildAgent\\Appveyor.MSBuildLogger.dll'])

def push_artifact(name):
  subprocess.call(['appveyor', 'PushArtifact', name])

if __name__ == '__main__':
  # Update build number
  build_number = os.getenv('APPVEYOR_BUILD_NUMBER')
  if build_number:
    with open(r'build.txt', 'w') as build_file:
      build_file.write('#define BUILD ' + build_number + '\n')
  # Build
  build('x86', 'Release')
  build('x64', 'Release')
  build('x86', 'Debug')
  build('x64', 'Debug')
  tag = build_number or ''
  # Zip binaries
  zipname = 'gcheapstat' + tag + '.zip';
  zip_binary(zipname, 'x86', 'Release')
  zip_binary(zipname, 'x64', 'Release')
  push_artifact(zipname)
  zipname = 'gcheapstat' + tag + 'd.zip';
  zip_binary(zipname, 'x86', 'Debug')
  zip_binary(zipname, 'x64', 'Debug')
  push_artifact(zipname)
  # Zip symbols
  zipname = 'gcheapstat' + tag + 'pdb.zip';
  zip_symbols(zipname, 'x86', 'Release')
  zip_symbols(zipname, 'x64', 'Release')
  zip_symbols(zipname, 'x86', 'Debug')
  zip_symbols(zipname, 'x64', 'Debug')
  push_artifact(ipname)
