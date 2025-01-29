# Make file to generate the 'optimized' versions of the panel SVGs from the sources
#
# This is required to embed font paths into the final SVG files
# Using svgo will optimize the SVG for filesize and performance
# Inkscape is used to export the font information to paths
# Install svgo with 'npm -g install svgo'

# Notes:
# $(wildcard *.svg) should be used instead of just *.svg
# $@ outputs the target name
# $? outputs all prereqs newer than the target
# $^ outputs all prereqs
# $< outputs the first prereq

INKSCAPE=inkscape
SVGO=svgo
SVGS=BufferedMult-dark.svg BufferedMult.svg DaisyChannel-dark.svg DaisyChannel.svg DaisyChannel2-dark.svg DaisyChannel2.svg DaisyChannelSends2-dark.svg DaisyChannelSends2.svg DaisyChannelVu-dark.svg DaisyChannelVu.svg DaisyMaster-dark.svg DaisyMaster.svg DaisyMaster2-dark.svg DaisyMaster2.svg Horsehair-dark.svg Horsehair.svg MasterMixer-dark.svg MasterMixer.svg UnityMix-dark.svg UnityMix.svg

all: $(SVGS)

# This is an example of building an svg file without the make vars so you can see a 'real example':
# MasterMixer.svg: src/MasterMixer.src.svg
#    $(INKSCAPE) src/MasterMixer.src.svg --export-plain-svg --export-type=svg --export-filename=MasterMixer-textpaths.svg --export-text-to-path
#    $(SVGO) -i MasterMixer-textpaths.svg -o MasterMixer.svg
#    rm MasterMixer-textpaths.svg

$(SVGS): %.svg: src/%.src.svg
	@echo "$(basename $@) $<"
	$(INKSCAPE) $< --export-plain-svg --export-type=svg --export-filename=$(basename $@)-textpaths.svg --export-text-to-path
	$(SVGO) -i $(basename $@)-textpaths.svg -o $@
	rm $(basename $@)-textpaths.svg

src/%.src.svg:
	touch $@
