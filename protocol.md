# common

- numbers and lengths are encoded in little-endian.

# message

```
| size | payload |
  4B
```

# request payload

```
| nstr | len1 | str1 | ... | lenn | strn |
  4B     4B                  4B

. nstr: number of strings
. leni: size of stri
```

# response payload

- **nil**:

```
| tag |
  1B
```

- **int64**:

```
| tag | int |
  1B    8B
```

- **double**:

```
| tag | dbl |
  1B    8B
```

- **string**:

```
| tag | len | str |
  1B    4B
```

- **array**:

```
| tag | len | items... |
  1B    4B
```

- **error**:

```
| tag | code | msg_size | msg |
  1B    4B     4B
```
