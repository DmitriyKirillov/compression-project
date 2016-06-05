TARGET_LIB(
        SOURCES DictHuffman.h DictHuffman.h
        LINK_DEPS library-common library-Bor library-Huffman
)

ADD_SUBDIRECTORY(test)
