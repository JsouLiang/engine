#!/bin/bash

upload_dsym_to_slardar() {
	echo "Start upload dSYM to HMD server"
	STATUS=$(curl "http://symbolicate.byted.org/slardar_ios_upload" -F "file=@${1}" -F "aid=13" -H "Content-Type: multipart/form-data" -w %{http_code} -v)
	echo "HMD server response: ${STATUS}"
}

dSYMInfoPlistPath=$(pwd)"/Info.plist"
cd ..

jcount=$1

if [ ! $jcount ]
then
    jcount=4
fi

tosDir=$2
if [ ! $tosDir ]
then 
	tosDir=$(git rev-parse HEAD)
fi

if [ ! $tosDir ]
then 
	tosDir='default'
fi

cd ..

for mode in 'debug' 'profile' 'release' 'release_dynamicart'
	do
#		hostDir=out/host_${mode}
		iOSArm64Dir=out/ios_${mode}
		iOSArmV7Dir=out/ios_${mode}_arm
		if [ "$mode" == "release_dynamicart" ]
		then
			iOSArmV7Dir=out/ios_release_arm_dynamicart
		fi
		iOSSimDir=out/ios_debug_sim
		cacheDir=out/tt_ios_${mode}
		dSYMInfoPlist=flutter/tt_build_tools/Info.plist

		[ -d $cacheDir ] && rm -rf $cacheDir
		mkdir $cacheDir

#		./flutter/tools/gn --runtime-mode=$mode
#		ninja -C $hostDir -j $jcount

		# 编译各种架构引擎
        if [ "$mode" == "release_dynamicart" ]
        then
            ./flutter/tools/gn --ios --runtime-mode=release --dynamicart
            ninja -C $iOSArm64Dir -j $jcount

            ./flutter/tools/gn --ios --runtime-mode=release --ios-cpu=arm --dynamicart
            ninja -C $iOSArmV7Dir -j $jcount
        else
            ./flutter/tools/gn --ios --runtime-mode=$mode
            ninja -C $iOSArm64Dir -j $jcount

            ./flutter/tools/gn --ios --runtime-mode=$mode --ios-cpu=arm
            ninja -C $iOSArmV7Dir -j $jcount
        fi

        ./flutter/tools/gn --ios --runtime-mode=debug --simulator
        ninja -C $iOSSimDir -j $jcount

		# 多种引擎架构合成一个
		lipo -create $iOSArm64Dir/Flutter.framework/Flutter $iOSArmV7Dir/Flutter.framework/Flutter $iOSSimDir/Flutter.framework/Flutter -output $cacheDir/Flutter

		# release模式引擎裁剪符号表
		if [ "$mode" == "release" -o "$mode" == "release_dynamicart" ]
		then
			echo "Generate dSYM"
			cd $cacheDir
			xcrun dsymutil -o Flutter.dSYM Flutter
			cp ${dSYMInfoPlistPath} Flutter.dSYM/Contents/Info.plist
			zip -rq Flutter.dSYM.zip Flutter.dSYM
			[ -e Flutter.dSYM ] && rm -rf Flutter.dSYM
			xcrun strip -x -S Flutter
			cd -
		fi

		cp -r $iOSArm64Dir/Flutter.framework $cacheDir/Flutter.framework
		mv $cacheDir/Flutter $cacheDir/Flutter.framework/Flutter

		# 多种架构gen_snapshot合成一个
		lipo -create $iOSArm64Dir/clang_x64/gen_snapshot $iOSArmV7Dir/clang_x86/gen_snapshot -output $cacheDir/gen_snapshot

		cp $iOSArm64Dir/Flutter.podspec $cacheDir/Flutter.podspec

		cd $cacheDir

		cd Flutter.framework
		zip -rq Flutter.framework.zip Flutter Headers icudtl.dat Info.plist Modules
		cd ..
		mv Flutter.framework/Flutter.framework.zip Flutter.framework.zip
		[ -d Flutter.framework ] && rm -rf Flutter.framework
		zip -rq artifacts.zip Flutter.framework.zip gen_snapshot Flutter.podspec
		[ -e Flutter.framework.zip ] && rm -rf Flutter.framework.zip
		[ -e gen_snapshot ] && rm -rf gen_snapshot
		[ -e Flutter.podspec ] && rm -rf Flutter.podspec

		cd ..
		cd ..

		modeDir=ios
		if [ "$mode" == "profile" ]
		then
			modeDir=ios-profile
		elif [ "$mode" == "release" ]
		then
			modeDir=ios-release
		elif [ "$mode" == "release_dynamicart" ]
		then
			modeDir=ios-dynamicart-release
		else
			modeDir=ios
		fi

		node ./flutter/tt_build_tools/tosUpload.js $cacheDir/artifacts.zip flutter/framework/$tosDir/$modeDir/artifacts.zip

		if [ "$mode" == "release" -o "$mode" == "release_dynamicart" ]
		then
			node ./flutter/tt_build_tools/tosUpload.js $cacheDir/Flutter.dSYM.zip flutter/framework/$tosDir/$modeDir/Flutter.dSYM.zip
            echo uploaded flutter/framework/$tosDir/$modeDir/Flutter.dSYM.zip
			upload_dsym_to_slardar "${cacheDir}/Flutter.dSYM.zip"
		fi
	done
