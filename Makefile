LIB= -loc -loctbstack -loc_logger -lconnectivity_abstraction -lcoap -luuid -lrt -lc_common -lpthread -lc -lz  
INCLUDEHEADER = -I/home/user/bbb/build/tmp/sysroots/beaglebone/lib -I/home/user/bbb/build/tmp/sysroots/beaglebone/usr/include -I/home/user/bbb/build/tmp/sysroots/beaglebone/usr/include/iotivity/resource -I/home/user/bbb/build/tmp/sysroots/beaglebone/usr/include/iotivity/resource/oc_logger -I/home/user/bbb/build/tmp/sysroots/beaglebone/usr/include/iotivity/resource/include/ -I/home/user/bbb/build/tmp/sysroots/beaglebone/usr/include/iotivity/resource/stack -I/home/user/bbb/build/tmp/sysroots/beaglebone/usr/include/iotivity/resource/csdk/stack/include -I/home/user/bbb/build/tmp/sysroots/beaglebone/usr/include/iotivity/resource/csdk/ocrandom/include -I/home/user/bbb/build/tmp/sysroots/beaglebone/usr/include/iotivity/resource/csdk/logger/include -I/home/user/bbb/build/tmp/sysroots/beaglebone/usr/include/iotivity/resource/oc_logger/include -Itestclient/cloud/digi_connector -Itestclient/resourcemgr -Itestclient/core -Itestclient/uimgr -Itestclient/databasemgr -I/home/user/bbb/build/tmp/sysroots/beaglebone/usr/include/libxml2 -Itestclient/intelligence/ 

help:
	@cat README 
build_all:  client server uiapp
clean_all: clean_cloudlib clean_xmllib clean_client clean_server clean_uiapp

cloudlib:
	make -C testclient/cloud/digi_connector
#xmllib:
#	sh xmlinstall.sh

client: clean_client out/resource_xml.o out/client.o out/resource_list.o out/error_response_handler.o out/Main.o out/process_schedular.o out/configuration.o out/MulticastRecv.o out/process_ui_request.o out/UiReceiver.o out/resource_database.o 
	${CXX} -o ihome  out/client.o out/resource_list.o out/Main.o out/process_schedular.o out/configuration.o out/MulticastRecv.o out/error_response_handler.o out/process_ui_request.o out/UiReceiver.o out/resource_xml.o  out/resource_database.o ${INCLUDEHEADER}  ${LIB} -L/home/user/LHOME_NEW/LHOME/lib -ldigicloud -L/home/user/LHOME/lib -ldigicloud	
out/resource_xml.o:
	${CXX} -std=c++11 ${INCLUDEHEADER} ${LIB} -o out/resource_xml.o -c testclient/databasemgr/resource_xml.cpp
out/resource_database.o:
	${CXX} -std=c++11 ${INCLUDEHEADER} ${LIB} -o out/resource_database.o -c testclient/databasemgr/resource_database.cpp
out/client.o:
	${CXX} -std=c++11 ${INCLUDEHEADER} ${LIB} -o out/client.o -c testclient/resourcemgr/client.cpp
out/resource_list.o:
	${CXX} -std=c++11 ${INCLUDEHEADER} ${LIB} -o out/resource_list.o -c testclient/resourcemgr/resource_list.cpp
out/error_response_handler.o:
	${CXX} -std=c++11 ${INCLUDEHEADER} ${LIB} -o out/error_response_handler.o -c testclient/resourcemgr/error_response_handler.cpp
out/Main.o:
	${CXX} -std=c++11 ${INCLUDEHEADER} ${LIB} -o out/Main.o -c testclient/core/Main.cpp
out/process_schedular.o:
	${CXX} -std=c++11 ${INCLUDEHEADER} ${LIB} -o out/process_schedular.o -c testclient/core/process_schedular.cpp
out/configuration.o:
	${CXX} -std=c++11 ${INCLUDEHEADER} ${LIB} -o out/configuration.o -c testclient/core/configuration.cpp
out/MulticastRecv.o:
	${CXX} -std=c++11 ${INCLUDEHEADER} ${LIB} -o out/MulticastRecv.o -c testclient/core/MulticastRecv.cpp
out/process_ui_request.o:
	${CXX} -std=c++11 ${INCLUDEHEADER} ${LIB} -o out/process_ui_request.o -c testclient/uimgr/process_ui_request.cpp
out/UiReceiver.o:
	${CXX} -std=c++11 ${INCLUDEHEADER} ${LIB} -o out/UiReceiver.o -c testclient/uimgr/UiReceiver.cpp

server:clean_server testserver/simpleserver.o testserver/getinterface.o
	${CXX} -std=c++11 ${INCLUDEHEADER} ${LIB} -o simpleserver testserver/simpleserver.o testserver/getinterface.o ${INCLUDELIB}  ${LIB}
testserver/simpleserver.o:
	${CXX} -std=c++11 ${INCLUDEHEADER} ${LIB} -o testserver/simpleserver.o -c testserver/simpleserver.cpp
testserver/getinterface.o:
	${CXX} -std=c++11 ${INCLUDEHEADER} ${LIB} -o testserver/getinterface.o -c testserver/getinterface.cpp

uiapp:clean_uiapp UIApp/UIApp.o
	${CXX} -std=c++11 ${INCLUDEHEADER} ${LIB} -o testUIapp UIApp/UIApp.o ${INCLUDELIB}  ${LIB}
UIApp/UIApp.o:
	${CXX} -std=c++11 ${INCLUDEHEADER} ${LIB} -o UIApp/UIApp.o -c UIApp/UIApp.cpp

clean_uiapp:
	rm -f UIApp/UIApp.o testUIapp
clean_client:
	rm -f out/* ihome 
clean_server:
	rm -f simpleserver testserver/simpleserver.o testserver/getinterface.o
clean_xmllib:
	make -C external/libxml2-2.9.2 distclean 
	rm -fr lib/xml_lib 
clean_cloudlib:
	make -C testclient/cloud/digi_connector clean
	rm -fr libdigicloud.a  lib/libdigicloud.so lib/libdigicloud.so.1 lib/libdigicloud.so.1.0  
