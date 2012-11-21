SCRIPTDIR=$(dirname $0)
cd $SCRIPTDIR

read -a VERSION -p "Version number: "

NAME=Doodle3D-osx-$VERSION
DIR=releases/$NAME
DATA=$DIR/data

if [ ! $VERSION = "" ]; then
	echo Creating $NAME...
	mkdir -p $DATA/gcode
	mkdir -p $DATA/doodles
	cp -r bin/data/images $DATA/
	cp bin/data/gcode/start.gcode $DATA/gcode/
	cp bin/data/gcode/end.gcode $DATA/gcode/
	cp bin/data/Doodle3D.ini $DATA/
	cp -r bin/Doodle3D.app $DATA/../
	cd $DIR
	zip -r -q ../$NAME.zip .
	cd ..
	rm -rf $NAME
fi

