
INKSCAPE=inkscape
SVGO=svgo
SVGS=MasterMixer.svg

all: $(SVGS)

MasterMixer.svg: src/MasterMixer.src.svg
	$(INKSCAPE) src/MasterMixer.src.svg --export-plain-svg=MasterMixer-textpaths.svg --export-text-to-path
	$(SVGO) -i MasterMixer-textpaths.svg -o MasterMixer.svg
	rm MasterMixer-textpaths.svg
