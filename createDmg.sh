SCRIPTDIR=$(dirname $0)
cd $SCRIPTDIR

read -a VERSION -p "Version number: "

SOURCE=Doodle3D
TITLE=Doodle3D

#rm pack.temp.dmg

#hdiutil create -srcfolder "${SOURCE}" -volname "${TITLE}" -fs HFS+ \
#      -fsargs "-c c=64,a=16,e=16" -format UDRW -size 10M pack.temp.dmg



#hdiutil convert "pack.temp.dmg" -format UDZO -imagekey zlib-level=9 -o Doodle3D.dmg

#sudo chmod -Rf go-w /Volumes/Doodle3D

#hdiutil attach -readwrite -noverify -noautoopen "pack.temp.dmg"

#device=$(hdiutil attach -readwrite -noverify -noautoopen "pack.temp.dmg" | egrep '^/dev/' | sed 1q | awk '{print $1}')


#echo device


#backgroundPictureName