I'v used a docker machine for this

```bash
docker run -ti -v $PWD:/app python bash
```

and then, inside the container

```bash
python --version
cd /app
git clone https://github.com/nanopb/nanopb.git
pip install python3-protobuf
pip install grpcio-tools
python nanopb/generator/nanopb_generator.py messages.proto -D $PWD/lib
```