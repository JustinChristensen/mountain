import Data.Bits

absTime :: Integer -> Integer
absTime tsc = ((tsc - tscBase) * scale) `shiftR` 32 + nsBase
    where 
        tscBase = 1727869341
        scale = 1478983228
        nsBase = 9245068404755

main :: IO ()
main = do
    putStrLn $ show $ absTime 2331666713
