.PHONY: all build engine frontend server test clean run

all: build

build: engine frontend

engine:
	$(MAKE) -C engine all

frontend:
	cd frontend && npm run build

test:
	$(MAKE) -C engine test
	node server/test_bridge.js
	node tests/e2e_verification.js

run: build
	node server/src/index.js

dev-server:
	node server/src/index.js

dev-frontend:
	cd frontend && npm run dev

clean:
	$(MAKE) -C engine clean
	rm -rf frontend/dist
