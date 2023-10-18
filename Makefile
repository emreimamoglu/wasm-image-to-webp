WEBP_DIR = libwebp
WEBP_TAR = libwebp.tar.gz
WEBP_URL = https://github.com/webmproject/libwebp/archive/refs/tags/v1.3.2.tar.gz

all: download_webp
	@emcc main.c \
	-o index.js \
	-O3 \
	-s ALLOW_MEMORY_GROWTH \
	-s WASM=1 \
	-s USE_ZLIB=1 \
	-s USE_LIBPNG=1 \
	-s USE_LIBJPEG=1 \
	-s EXPORTED_RUNTIME_METHODS='["cwrap"]' \
	-I $(WEBP_DIR) \
	$(WEBP_DIR)/src/{dec,dsp,demux,enc,mux,utils}/*.c \
	$(WEBP_DIR)/sharpyuv/*.c \

download_webp:
	@if [ ! -d "$(WEBP_DIR)" ]; then \
		mkdir -p $(WEBP_DIR); \
		curl -L -o $(WEBP_TAR) $(WEBP_URL); \
		tar -xzf $(WEBP_TAR) -C $(WEBP_DIR) --strip-components=1; \
		rm $(WEBP_TAR); \
	fi
