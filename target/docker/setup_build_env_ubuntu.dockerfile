#Dockerfile to compile InterSpec on (installs correct versions of Wt and Boost)

FROM alpine:3.9

#ENV http_proxy wwwproxy.ca.sandia.gov:80
#ENV https_proxy wwwproxy.ca.sandia.gov:80


ADD . /InterSpec

WORKDIR /tmp



#since this is just to build things, we'll use lots of run commands to d=take advantage of
# docker caching

#gcc version 8.2.0
#cmake version 3.13.0
RUN apk add build-base binutils gcc abuild cmake linux-headers zlib-dev musl-dev libc-dev
RUN mkdir build; cd build \
&& wget https://dl.bintray.com/boostorg/release/1.65.1/source/boost_1_65_1.tar.gz \
&& tar -xzvf boost_1_65_1.tar.gz \
&& cd boost_1_65_1 \
&& ./bootstrap.sh --without-icu --without-libraries=python,mpi,container,type_erasure,graph,graph_parallel,log,coroutine,stacktrace,test,wave --prefix=/usr/local \
&& ./b2 variant=release link=static threading=multi runtime-link=static -j8 install \
&& cd .. \
&& rm -rf boost_1_65_1 boost_1_65_1.tar.gz

RUN wget https://github.com/emweb/wt/archive/3.3.4.tar.gz \
&& tar -xzvf 3.3.4.tar.gz && cd wt-3.3.4 \
&& cp src/Wt/Dbo/backend/amalgamation/sqlite3.h /usr/local/include/ \
&& patch -b src/Wt/Render/CssParser.C /InterSpec/target/patches/wt/3.3.4/CssParser.C.patch \
&& patch -b src/http/RequestParser.C /InterSpec/target/patches/wt/3.3.4/RequestParser.C.patch \
&& mkdir build && cd build \
&& cmake -DWT_CPP_11_MODE="-std=c++11" -DWT_SIGNALS_IMPLEMENTATION="boost.signals2" \
      -DENABLE_SSL=OFF -DSHARED_LIBS=OFF -DCMAKE_BUILD_TYPE=Release \
      -DCONNECTOR_FCGI=OFF -DCONNECTOR_HTTP=ON -DBUILD_EXAMPLES=OFF \
      -DINSTALL_RESOURCES=ON -DENABLE_LIBWTTEST=ON -DENABLE_LIBWTDBO=ON \
      -DENABLE_QT4=OFF -DENABLE_SQLITE=ON -DENABLE_EXT=OFF -DENABLE_OPENGL=ON \
      -DBOOST_PREFIX=/usr/local \
      .. \
&& make -j4 install \
&& cd ../../.. \
&& rm -rf build

# && patch -b src/Wt/Dbo/ptr_impl.h /InterSpec/target/patches/wt/3.3.4/ptr_impl.h.patch \
#Other Wt options we'll accept defaults on: ENABLE_HARU(ON), ENABLE_PANGO(ON), ENABLE_POSTGRES(ON), 
#   ENABLE_FIREBIRD(ON), ENABLE_MYSQL(ON), WT_NO_STD_LOCALE(OFF), WT_NO_STD_WSTRING(OFF)


#RUN mkdir build_interspec \
#&& cd build_interspec \
#&& cmake -DCMAKE_BUILD_TYPE="Release" -DBUILD_FOR_WEB_DEPLOYMENT=ON -DENABLE_RESOURCE_UPDATES=OFF \
#         -DBUILD_AS_LOCAL_SERVER=OFF -DUSE_SPECRUM_FILE_QUERY_WIDGET=OFF -DTRY_TO_STATIC_LINK=ON \
####         -DCMAKE_EXE_LINKER_FLAGS="-static -static-libstdc++ -static-libgcc"
#         /interspec \
#&& make -j4
#Doing the static linking causes the EXE to crash immediately.  Maybe this is to to PIE stuff?
#cd .. && mkdir InterSpec && cd InterSpec \
#&& cp ../build_interspec/bin/InterSpec.exe . \
#&& cp -r ../build_interspec/resources . \
#&& cp -r /interspec/InterSpec_resources . \
#&& cp -r /interspec/example_spectra . \
#&& cp -r /interspec/data .
#

#CMD ["/bin/bash"]
