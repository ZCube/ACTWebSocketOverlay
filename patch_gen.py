from subprocess import check_output
import re
import os
import io
import glob
import sys
import zipfile
import filelock
import json
import hashlib
import bz2
import lzma
import shutil

lock_filename = ".filelock"

###############################################
out = str(check_output(["git", "describe", "--long"]).decode("utf-8")).replace('-', '.', 1).replace("\r","").replace("\n","")
arr = out.split(".")
var = arr[0:-1]
if len(var) < 3:
  var = var + ['0'] * (3 - len(arr))

vars = var + [arr[-1].split('-')[0]]
varl = var + [arr[-1]]

def tagOnly():
  return ".".join(var[0:3])

def tagOnlyWithComma():
  return ".".join(var[0:3])

def longVersion():
  return ".".join(varl)

def longVersionWithComma():
  return ",".join(varl)
  
def shortVersion():
  return ".".join(vars)

def shortVersionWithComma():
  return ",".join(vars)
###############################################

def genSHA1Hash(file):
  BUF_SIZE = 65536  # lets read stuff in 64kb chunks!

  sha1 = hashlib.sha1()

  with open(file, 'rb') as f:
    while True:
      data = f.read(BUF_SIZE)
      if not data:
        break
      sha1.update(data)
  return sha1.hexdigest()
  
def getHeadFileSet():
  out = str(check_output(["git", "ls-files"]).decode("utf-8")).replace("\r", "")
  arr = out.split("\n")
  return set([x for x in arr if x != '' and x != None])
  
def getRevisionList(branch="master", count=10):
  out = str(check_output(["git", "rev-list", "--all", "--max-count", str(count), "--branches="+branch, "--", "."]).decode("utf-8")).replace("\r", "")
  arr = out.split("\n")
  return [x for x in arr if x != '' and x != None]
  
def getDiffFileSetToHead(revision):
  out = str(check_output(["git", "diff", "--name-only", revision, "HEAD"]).decode("utf-8")).replace("\r", "")
  arr = out.split("\n")
  return set([x for x in arr if x != '' and x != None])
  
def getDiffFileStatusToHead(revision):
  out = str(check_output(["git", "diff", "--name-status", revision, "HEAD"]).decode("utf-8")).replace("\r", "")
  arr = out.split("\n")
  return set([x.split("\t") for x in arr if x != '' and x != None])
  
def getDiffModifiedFileSetToHead(revision):
  out = str(check_output(["git", "diff", "--name-status", revision, "HEAD"]).decode("utf-8")).replace("\r", "")
  arr = out.split("\n")
  return set([x.split("\t")[1] for x in arr if x != '' and x != None and x.split("\t")[0] !='D'])
  
def getDiffDeletedFileSetToHead(revision):
  out = str(check_output(["git", "diff", "--name-status", revision, "HEAD"]).decode("utf-8")).replace("\r", "")
  arr = out.split("\n")
  return set([x.split("\t")[1] for x in arr if x != '' and x != None and x.split("\t")[0] =='D'])
  
def do(outdir, version, description):
  print(getHeadFileSet())
  revisions = getRevisionList("master", 10) #1
  headRevision = revisions[0]
  
  dir = outdir+'/'+headRevision
  patchinfo_json = outdir+'/info.json'
  patchinfo_json_lock = outdir+'/info.json.lock'
  
  if not os.path.exists(dir):
    os.makedirs(dir)
  
  print(revisions)
  revisionFilesetMap = {}
  revisionFilesetList = []
  fileSet = getHeadFileSet()
  revisionFilesetMap[revisions[0]] = fileSet
  revisionFilesetList.append(fileSet)
  
  #2
  for revision in revisions[1:]: # 0 == HEAD
    fileSet = getDiffModifiedFileSetToHead(revision)
    revisionFilesetMap[revision] = fileSet
    #3
    if fileSet not in revisionFilesetList:
      revisionFilesetList.append(fileSet)
      
  print(revisionFilesetMap)
  print(revisionFilesetList)
  
  #4
  revisionFilesetHashList = []
  revisionFilesetSizeList = []
  for i in range(len(revisionFilesetList)):
    zipname = dir + '/'+str(i)+'.zip'
    #with zipfile.ZipFile(zipname, 'w', zipfile.ZIP_DEFLATED) as myzip:
    with zipfile.ZipFile(zipname, 'w', zipfile.ZIP_LZMA) as myzip:
    # with lzma.open(zipname, 'w') as myzip:
      for file in revisionFilesetList[i]:
        myzip.write(file)
    revisionFilesetHashList.append(genSHA1Hash(zipname))
    revisionFilesetSizeList.append(os.path.getsize(zipname))
    
  #5
  patches = {}
  hashes = {}
  for revision in revisions:
    idx = revisionFilesetList.index(revisionFilesetMap[revision])
    patches[revision] = {'idx':idx, 'hash': revisionFilesetHashList[idx], 'removes':list(getDiffDeletedFileSetToHead(revision)), 'size': revisionFilesetSizeList[idx], 'path': revisions[0]+'/'+str(idx)+'.zip'}
  info = {'revision':revisions[0], 'patches':patches, 'version':version, 'description':description}
  print(info)
  
  #6
  
  
  #7
  with io.open(patchinfo_json, 'w', encoding='utf-8') as f:
    # while True:
      # try:
          # fcntl.flock(f, fcntl.LOCK_EX | fcntl.LOCK_NB)
          # break
      # except IOError as e:
          # # raise on unrelated IOErrors
          # if e.errno != errno.EAGAIN:
              # raise
          # else:
              # time.sleep(0.1)
      # except OSError as e:
          # # raise on unrelated IOErrors
          # if e.errno != errno.EAGAIN:
              # raise
          # else:
              # time.sleep(0.1)
    f.write(json.dumps(info))
    # fcntl.flock(f, fcntl.LOCK_UN)

  
  revisions20 = getRevisionList("master", 20)
  revisions10 = getRevisionList("master", 10)
  print(revisions20)
  print(revisions10)
  print((set(revisions20) - set(revisions10)))
  for rev in (set(revisions20) - set(revisions10)):
    print(rev)
    del_dir = outdir+'/'+rev
    if(os.path.exists(del_dir)):
      shutil.rmtree(del_dir)
  #print(getDiffFileListToHead())
  #1. get revision List
  #git rev-list --all --max-count 10 --branches=master
  #2. get Diff File List
  #git diff --name-only rev HEAD
  #3. group file list
  #4. zip grouped file list
  #5. matching revision to file list
  #6. publish zips
  #7. publish revision with file lock
  #8. del previos patchs n before
  

if __name__ == '__main__':

  if len(sys.argv) < 3:
    print("patch_gen.py [src] [patch]")
    sys.exit(0)
  #do('./patch')
  lock = filelock.FileLock(lock_filename)
  dir = os.path.abspath(sys.argv[1])
  outdir = os.path.abspath(sys.argv[2])
  version = longVersion()
  description = None
  if len(sys.argv) > 3:
    description = sys.argv[3]
  os.chdir(dir)
  with lock:
    do(outdir, version, description)
  os.chdir(dir)
