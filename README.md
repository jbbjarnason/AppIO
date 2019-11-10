# AppIO
Application io developer framework for linux.

## Dependancies

```bash
$ sudo apt install libboost-all-dev libzmq3-dev libazmq-dev nlohmann-json3-dev rapidjson-dev
```

## Example
### endtoend
```bash
$ ./endtoend --name default
```
Thereafter a folder is created at 
```bash
$ cd ~/.industry/endtoend/default
$ ls # This should return config.json and folder ipc
```
config.json witholds the application config. Currently is only loaded on startup. Note: also specifies pub/sub topics.