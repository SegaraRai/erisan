# erisan

ERISA-N符号のエンコードとデコードを行うプログラム

## 使い方

```text
erisan [e|d] infile outfile
```

第一引数を`e`にするとエンコードを、`d`にするとデコードを行います。  
infileは元ファイルを、outfileは出力先ファイルを指定します。  
infileに`-`を指定すると標準入力を読み込みます。  
outfileに`-`を指定すると標準出力に書き出します。  
