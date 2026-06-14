# message format:

```
| size | payload |
  4B     size B

. size: little-endian
```

# request payload format:

```
| nstr | len1 | str1 | ... | lenn | strn |
  4B     4B                  4B

. nstr: number of strings, little-endian
. leni: size of stri, little-endian
```

# response payload format:

```
| status | data |
  4B

. status: little-endian
```
