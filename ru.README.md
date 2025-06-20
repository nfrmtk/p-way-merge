Сборка: 

Пререквизиты:

```bash
sudo apt-get install cmake g++ make ninja-build libgtest-dev 
```

Чтобы собрать тест с именем merge_test, нужно указать merge_test_ut в качестве target в вызове cmake --build, например эти команды собирают тесты из файла /pmerge/tests/merge_test.cpp: 

bash ```
cd p-way-merge 
mkdir build && cd build 
cmake -DCMAKE_BUILD_TYPE=Debug ..
cmake --build . --target merge_test_ut
```

На данный момент работают тесты merge_test и pack_test.