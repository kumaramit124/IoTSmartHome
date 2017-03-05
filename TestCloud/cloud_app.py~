#!/usr/bin/python
#
# ***************************************************************************
# Copyright (c) 2013 Digi International Inc.,
# All rights not expressly granted are reserved.
# 
# This Source Code Form is subject to the terms of the Mozilla Public
# License, v. 2.0. If a copy of the MPL was not distributed with this file,
# You can obtain one at http://mozilla.org/MPL/2.0/.
# 
# Digi International Inc. 11001 Bren Road East, Minnetonka, MN 55343
#
# ***************************************************************************
# get_file.py
# Get a file from Device Cloud storage
# -------------------------------------------------
# Usage: get_file.py <Username> <Password> <Device ID> [<Device Cloud URL>]
# -------------------------------------------------

import httplib
import base64
import sys
import re

def Usage():
    print '    0 - Get the device parameter'
    print '    1 - Set The device parameter'
    print '    2 - Put device parameter - do not use'
    print '    3 - get resource list'

def PostMessage(username, password, device_id, target, cmd, uri, hostaddress, roomname, state, intensity, cloud_url):
    # create HTTP basic authentication string, this consists of
    # "username:password" base64 encoded
    auth = base64.encodestring("%s:%s"%(username,password))[:-1]
    
    # device request message to send to Device Cloud
    message = """<sci_request version="1.0">
        <data_service>
            <targets>
                <device id="%s"/>
            </targets>
            <requests>
            <device_request target_name="%s">%s %s %s %s %s %s</device_request>
            </requests>
        </data_service>
    </sci_request>
    """%(device_id, target, cmd, uri, hostaddress, roomname, state, intensity)
    
    # to what URL to send the request with a given HTTP method
    webservice = httplib.HTTP(cloud_url,80)
    webservice.putrequest("POST", "/ws/sci")
    
    # add the authorization string into the HTTP header
    webservice.putheader("Authorization", "Basic %s"%auth)
    webservice.putheader("Content-type", "text/xml; charset=\"UTF-8\"")
    webservice.putheader("Content-length", "%d" % len(message))
    webservice.endheaders()
    webservice.send(message)
    
    # get the response
    statuscode, statusmessage, header = webservice.getreply()
    response_body = webservice.getfile().read()
    
    # print the output to standard out
    if statuscode == 200:
        print '\nResponse:'
        print response_body
    else:
        print '\nError: %d %s' %(statuscode, statusmessage)
    
    webservice.close()

def GetMessage(username, password, device_id, cloud_url):
    # create HTTP basic authentication string, this consists of
    # "username:password" base64 encoded
    auth = base64.encodestring("%s:%s"%(username,password))[:-1]

    # device request message to send to Device Cloud
    path = """/ws/FileData/~/%s/device.txt"""%(device_id)

    # to what URL to send the request with a given HTTP method
    webservice = httplib.HTTP(cloud_url,80)
    webservice.putrequest("GET", path)

    # add the authorization string into the HTTP header
    webservice.putheader("Authorization", "Basic %s"%auth)
    webservice.putheader("Accept", "text/html; charset=\"UTF-8\"")
    #webservice.putheader("Content-length", "%d" % len(message))
    webservice.endheaders()
    #webservice.send(message)

    # get the response
    statuscode, statusmessage, header = webservice.getreply()
    response_body = webservice.getfile().read()

    # print the output to standard out
    if statuscode == 200:
        print response_body
    else:
        print '\nError: %d %s' %(statuscode, statusmessage)

    webservice.close()

def main(argv):
    username = "Manjupriya"
    password = "Manju.1990"
    deviceId = "00000000-00000000-2C325DFF-FFD589AF"
    cloud_url = "login.etherios.com"
    target = "myTarget"
    uri = ""
    roomname = ""
    hostaddress = ""
    state = ""
    intensity = ""
    var = 1
    while var == 1:
       Usage()
       x = int(input("Please enter : "))
       if x == 0:
          uri = raw_input('uri::') 
          hostaddress=raw_input('hostaddress::')
          PostMessage(username, password, deviceId, target, "0",uri,hostaddress,roomname,state,intensity, cloud_url)
       elif x == 1:
          uri = raw_input('uri::')
          hostaddress=raw_input('hostaddress::')
          roomname = raw_input('roomname::')
          state = raw_input('state::')
          intensity = raw_input('intensity::')
          PostMessage(username, password, deviceId, target, "1",uri,hostaddress,roomname,state,intensity, cloud_url)
       elif x == 3:
          PostMessage(username, password, deviceId, target, "3",uri,hostaddress,roomname,state,intensity, cloud_url)
          GetMessage(username,password,deviceId , cloud_url)
       else:
          print('Wrong command')
      
if __name__ == "__main__":
    sys.exit(main(sys.argv[1:]))

