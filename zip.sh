name=g08-64018-64288-64646
dir=${PWD##*/}

make clean > /dev/null # clean obj files
rm -f $name.zip > /dev/null # clean previous zips/folders
rm -rf $name > /dev/null
rm -f $name > /dev/null

# copy files to new folder
cd ..
rm -rf $name > /dev/null
mkdir $name
cp -r $dir/* $name

# zip files
zip -r "$dir/$name.zip" $name > /dev/null
rm -rf $name > /dev/null

# take md5 hash
md5 "$dir/$name.zip"