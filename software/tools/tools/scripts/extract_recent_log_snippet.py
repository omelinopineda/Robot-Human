import lcm

def seekToTime(log,desiredTime):
  totalBytes = log.size()
  bytes1 = 0
  bytes2 = totalBytes
  log.seek(0)
  time1 = log.read_next_event().timestamp
  if (time1 > desiredTime):
    print time1, desiredTime
    return False
  
  while (True):
    bytesCur = (bytes1+bytes2)/2
    log.seek(bytesCur)
    time = log.read_next_event().timestamp
    if (time < desiredTime):
      if (bytes1 == bytesCur):
        log.seek(bytesCur)
        return True
      bytes1 = bytesCur
    elif (time > desiredTime):
      if (bytes2 == bytesCur):
        log.seek(bytesCur)
        return True
      bytes2 = bytesCur
    else:
      log.seek(bytesCur)
      return True

def findLastTimestamp(log):
  totalBytes = log.size()
  log.seek(totalBytes)
  curStep = 1
  time = -1
  while True:
    if (time > 0):
      break
    event = log.read_next_event()
    if event is None:
      log.seek(totalBytes-curStep)
      curStep = 2*curStep
    else:
      time = event.timestamp
      while True:
        event = log.read_next_event()
        if event is None:
          break
  return time

def chopLog(inLog,outLog,timeMin,timeMax):
  seekToTime(inLog,timeMin)
  while True:
    event = inLog.read_next_event()
    if event is None:
      break
    time = event.timestamp
    if (time > timeMax):
      break
    outLog.write_event(event.timestamp, event.channel, event.data)

def copyEndOfLog(inFileName,outFileName,timeSec):
  inLog = lcm.EventLog(inFileName,'r')
  outLog = lcm.EventLog(outFileName,'w')
  timeMax = findLastTimestamp(inLog)  
  timeMin = timeMax-int(timeSec*1e6)
  print timeMin,timeMax
  chopLog(inLog,outLog,timeMin,timeMax)
  inLog.close()
  outLog.close()

def findNewestFile(pattern):
  import glob
  import os
  files = glob.glob(pattern)
  if len(files)==0:
    return None
  times = [os.path.getmtime(f) for f in files]
  maxIndex = times.index(max(times))
  return files[maxIndex]
  
def go(pattern,timeSec):
  import datetime
  inFileName = findNewestFile(pattern)
  if inFileName is None:
    print 'error: no files matched pattern ' + pattern
    return False
  inLog = lcm.EventLog(inFileName,'r')
  timeMax = findLastTimestamp(inLog)
  inLog.close()
  timeStr = datetime.datetime.fromtimestamp(timeMax/1e6).strftime('%Y-%m-%d_%H-%M-%S')
  outFileName = inFileName + '_snippet_' + str(int(timeSec)) + '_' + timeStr
  copyEndOfLog(inFileName,outFileName,timeSec)
  return True

if __name__=='__main__':
  import argparse
  parser = argparse.ArgumentParser(description='copy data from end of lcm log')
  parser.add_argument('-i', dest='inputpattern', required=True, help='input log directory')
  parser.add_argument('-d', dest='duration', type=float, default=60.0, help='duration of output log')
  args = parser.parse_args()
  result = go(args.inputpattern,args.duration)
  if result:
    print 'done'
  else:
    print 'failed'
