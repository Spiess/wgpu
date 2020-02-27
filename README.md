# wgpu
A command line utility to show who is using which GPU and what they are doing - similar to w.

## Installation

```shell script
pip install git+https://github.com/spiess/wgpu.git
```

## Usage

If installed in a virtual environment:

```shell script
wgpu
```

Otherwise:

```shell script
python3 -m wgpu
```

To highlight a specific GPU (e.g. GPU with ID 1):

```shell script
wgpu --gpu 1
```

To highlight GPUs and processes used by a specific user (e.g. florian):

```shell script
wgpu --username florian
```

If used with `watch` use `--color` to enable highlighting:

```shell script
watch --color -n 1 wgpu --user florian
```