libandroid-shm:
	$(CC) $(CFLAGS) -fPIC -shared -g -I./include cutils/ashmem-dev.c libandroid-shm.c -Wl,-wrap=mmap $(LDFLAGS) -o libandroid-shm$(LIBSUFFIX).so

shm-launch:
	$(CC) $(CFLAGS) -fPIC  -I./include shm-launch.c $(LDFLAGS) -o shm-launch -L./ -landroid-shm$(LIBSUFFIX)

test:
	$(CC) $(CFLAGS) -I./include tests/test-server.c $(LDFLAGS) -o test-server -L./ -lrt
	$(CC) $(CFLAGS) -I./include tests/test-client.c $(LDFLAGS) -o test-client -L./ -lrt

clean:
	rm *.so shm-launch test-client test-server

all: libandroid-shm shm-launch test

install:
	install -D --mode 755 libandroid-shm$(LIBSUFFIX).so $(DESTDIR)/$(PREFIX)/lib/$(TARGET)/libandroid-shm$(LIBSUFFIX).so
	install -D --mode 755 shm-launch $(DESTDIR)/$(PREFIX)/bin/shm-launch
	install -D --mode 644 include/ashm.h $(DESTDIR)/usr/include/
