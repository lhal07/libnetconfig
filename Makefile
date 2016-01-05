CC=g++
OBJ_LINK=-fPIC -Wall -c
SHR_LINK=-shared
LINK=ln -s
CP=cp -rf
RM=rm -f
RMDIR=rm -rf
MKDIR=mkdir -p
LIB=netconfig
VER=0.1-0
PREFIX=/usr
LIBNAME=lib$(LIB)
SRC_DIR=src
LIB_DIR=lib
BIN_DIR=bin
INC_DIR=include
PKG_DIR=debian
PACKAGE=$(LIBNAME)-$(VER).deb

all: lib test package

bin_dir:
	$(MKDIR) bin

lib_dir:
	$(MKDIR) lib

lib: $(LIB_DIR)/$(LIBNAME).so 

$(LIB_DIR)/$(LIBNAME).so: $(SRC_DIR)/$(LIBNAME).o lib_dir
	$(CC) $(SHR_LINK) -Wl,-soname,$(LIBNAME).so.0 -I$(INC_DIR) -o $(LIB_DIR)/$(LIBNAME).so.0.0 $(SRC_DIR)/$(LIBNAME).o -lc
	$(RM) $(LIB_DIR)/$(LIBNAME).so.0
	$(LINK) $(LIBNAME).so.0.0 $(LIB_DIR)/$(LIBNAME).so.0
	$(RM) $(LIB_DIR)/$(LIBNAME).so
	$(LINK) $(LIBNAME).so.0 $(LIB_DIR)/$(LIBNAME).so

$(SRC_DIR)/$(LIBNAME).o:
	$(CC) $(OBJ_LINK) -I$(INC_DIR) $(SRC_DIR)/$(LIBNAME).cpp -o $(SRC_DIR)/$(LIBNAME).o

test: $(BIN_DIR)/test

$(BIN_DIR)/test: $(SRC_DIR)/test.cpp bin_dir
	$(CC) -I$(INC_DIR) $(SRC_DIR)/test.cpp -Llib -l$(LIB) -lpthread -o $(BIN_DIR)/test

package: lib
	$(MKDIR) $(PKG_DIR)/$(LIBNAME)/$(PREFIX)/lib
	$(MKDIR) $(PKG_DIR)/$(LIBNAME)/$(PREFIX)/include
	$(CP) $(LIB_DIR)/$(LIBNAME).so.0.0 $(PKG_DIR)/$(LIBNAME)/$(PREFIX)/lib/
	$(CP) $(INC_DIR) $(PKG_DIR)/$(LIBNAME)/$(PREFIX)
	dpkg-deb --build $(PKG_DIR)/$(LIBNAME)
	mv $(PKG_DIR)/$(LIBNAME).deb $(PKG_DIR)/$(PACKAGE)

install: lib
	$(CP) $(LIB_DIR)/$(LIBNAME).so.0.0 $(PREFIX)/$(LIB_DIR)
	$(RM) $(PREFIX)/$(LIB_DIR)/$(LIBNAME).so.0
	$(LINK) $(PREFIX)/$(LIB_DIR)/$(LIBNAME).so.0.0 $(PREFIX)/$(LIB_DIR)/$(LIBNAME).so.0
	$(RM) $(PREFIX)/$(LIB_DIR)/$(LIBNAME).so
	$(LINK) $(PREFIX)/$(LIB_DIR)/$(LIBNAME).so.0 $(PREFIX)/$(LIB_DIR)/$(LIBNAME).so
	$(CP) $(INC_DIR)/netcfg $(PREFIX)/$(INC_DIR)

uninstall:
	$(RM) $(PREFIX)/$(LIB_DIR)/$(LIBNAME).so
	$(RM) $(PREFIX)/$(LIB_DIR)/$(LIBNAME).so.0
	$(RM) $(PREFIX)/$(LIB_DIR)/$(LIBNAME).so.0.0
	$(RMDIR) $(PREFIX)/$(INC_DIR)/netcfg

clean:
	$(RM) $(SRC_DIR)/$(LIBNAME).o
	$(RMDIR) $(LIB_DIR)
	$(RMDIR) $(BIN_DIR)
	$(RM) $(PKG_DIR)/$(PACKAGE)
	$(RMDIR) $(PKG_DIR)/$(LIBNAME)/$(PREFIX)
