I'v used a docker machine for this

```bash
```

and then, inside the container

```bash
python --version
cd /app
pip install python3-protobuf
pip install grpcio-tools
python nanopb/generator/nanopb_generator.py messages.proto -D $PWD/lib
```