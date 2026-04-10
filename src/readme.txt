コンパイルするにはPSPSDKが必要です。PS2SDKではできません。

通常版のビルド:                        make
日本語版のビルド:                      make WITH_JAPANESE=1
日本語版(JIS第二水準サポート)のビルド: make WITH_JAPANESE=1 WITH_JIS_LEVEL2=1

デバッグ用のビルド:              make WITH_EXCEPTION_HANDLER=1
PSPLINKへのデバッグ文字列出力用: make WITH_PSPLINK_DEBUG=1
