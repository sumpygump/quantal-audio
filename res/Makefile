
INKSCAPE=inkscape
SVGO=svgo
SVGS=MasterMixer.svg BufferedMult.svg UnityMix.svg DaisyChannel.svg DaisyChannel2.svg DaisyChannelSends2.svg DaisyChannelVu.svg DaisyMaster.svg DaisyMaster2.svg Horsehair.svg

all: $(SVGS)

MasterMixer.svg: src/MasterMixer.src.svg
	$(INKSCAPE) src/MasterMixer.src.svg --export-plain-svg --export-type=svg --export-filename=MasterMixer-textpaths.svg --export-text-to-path
	$(SVGO) -i MasterMixer-textpaths.svg -o MasterMixer.svg
	rm MasterMixer-textpaths.svg

BufferedMult.svg: src/BufferedMult.src.svg
	$(INKSCAPE) src/BufferedMult.src.svg --export-plain-svg --export-type=svg --export-filename=BufferedMult-textpaths.svg --export-text-to-path
	$(SVGO) -i BufferedMult-textpaths.svg -o BufferedMult.svg
	rm BufferedMult-textpaths.svg

UnityMix.svg: src/UnityMix.src.svg
	$(INKSCAPE) src/UnityMix.src.svg --export-plain-svg --export-type=svg --export-filename=UnityMix-textpaths.svg --export-text-to-path
	$(SVGO) -i UnityMix-textpaths.svg -o UnityMix.svg
	rm UnityMix-textpaths.svg

DaisyChannel.svg: src/DaisyChannel.src.svg
	$(INKSCAPE) src/DaisyChannel.src.svg --export-plain-svg --export-type=svg --export-filename=DaisyChannel-textpaths.svg --export-text-to-path
	$(SVGO) -i DaisyChannel-textpaths.svg -o DaisyChannel.svg
	rm DaisyChannel-textpaths.svg

DaisyChannel2.svg: src/DaisyChannel2.src.svg
	$(INKSCAPE) src/DaisyChannel2.src.svg --export-plain-svg --export-type=svg --export-filename=DaisyChannel2-textpaths.svg --export-text-to-path
	$(SVGO) -i DaisyChannel2-textpaths.svg -o DaisyChannel2.svg
	rm DaisyChannel2-textpaths.svg

DaisyChannelSends2.svg: src/DaisyChannelSends2.src.svg
	$(INKSCAPE) src/DaisyChannelSends2.src.svg --export-plain-svg --export-type=svg --export-filename=DaisyChannelSends2-textpaths.svg --export-text-to-path
	$(SVGO) -i DaisyChannelSends2-textpaths.svg -o DaisyChannelSends2.svg
	rm DaisyChannelSends2-textpaths.svg

DaisyChannelVu.svg: src/DaisyChannelVu.src.svg
	$(INKSCAPE) src/DaisyChannelVu.src.svg --export-plain-svg --export-type=svg --export-filename=DaisyChannelVu-textpaths.svg --export-text-to-path
	$(SVGO) -i DaisyChannelVu-textpaths.svg -o DaisyChannelVu.svg
	rm DaisyChannelVu-textpaths.svg

DaisyMaster.svg: src/DaisyMaster.src.svg
	$(INKSCAPE) src/DaisyMaster.src.svg --export-plain-svg --export-type=svg --export-filename=DaisyMaster-textpaths.svg --export-text-to-path
	$(SVGO) -i DaisyMaster-textpaths.svg -o DaisyMaster.svg
	rm DaisyMaster-textpaths.svg

DaisyMaster2.svg: src/DaisyMaster2.src.svg
	$(INKSCAPE) src/DaisyMaster2.src.svg --export-plain-svg --export-type=svg --export-filename=DaisyMaster2-textpaths.svg --export-text-to-path
	$(SVGO) -i DaisyMaster2-textpaths.svg -o DaisyMaster2.svg
	rm DaisyMaster2-textpaths.svg

Horsehair.svg: src/Horsehair.src.svg
	$(INKSCAPE) src/Horsehair.src.svg --export-plain-svg --export-type=svg --export-filename=Horsehair-textpaths.svg --export-text-to-path
	$(SVGO) -i Horsehair-textpaths.svg -o Horsehair.svg
	rm Horsehair-textpaths.svg
