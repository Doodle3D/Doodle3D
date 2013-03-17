SCRIPTDIR=$(dirname $0)
cd $SCRIPTDIR

rm -rf Doodle3D
mkdir Doodle3D
mkdir Doodle3D/.background
cp bg.png Doodle3D/.background
cp -a ../../bin/Doodle3D.app Doodle3D/
ln -s /Applications Doodle3D/Applications

VERSION=$(defaults read `pwd`/../../openFrameworks-Info CFBundleShortVersionString)
#VERSION=$(cat Doodle3D/Doodle3D.app/Contents/Resources/VERSION)

DMG_NAME=Doodle3D-osx-$VERSION.dmg
rm $DMG_NAME

hdiutil detach /Volumes/Doodle3D/
rm temp.dmg

hdiutil create -srcfolder Doodle3D -volname Doodle3D -fs HFS+ \
	-fsargs "-c c=64,a=16,e=16" -format UDRW -size 20M temp.dmg

device=$(hdiutil attach -readwrite -noverify -noautoopen "temp.dmg" | \
         egrep '^/dev/' | sed 1q | awk '{print $1}')

echo '
	tell application "Finder"
		tell disk "Doodle3D"
			open
			set theViewOptions to the icon view options of container window
			tell container window
				set current view to icon view
				set arrangement of theViewOptions to not arranged
				set icon size of theViewOptions to 72
				set background picture of theViewOptions to file ".background:bg.png"
				set toolbar visible to false
				set statusbar visible to false
				set the bounds to {400, 100, 850, 430}
				set position of item "Applications" to {340, 190}
				set position of item "Doodle3D" to {100, 190}
			end tell
			close
			open
		end tell
	end tell
' | osascript

rm -r Doodle3D
hdiutil detach /Volumes/Doodle3D/
hdiutil convert temp.dmg -format UDZO -imagekey zlib-level=9 -ov -o $DMG_NAME
rm temp.dmg

