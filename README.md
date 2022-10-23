# search-server
Программная реализация ядра поискового сервера.
Поисковый сервер хранит стоп-слова, документы и позволяет осуществить быстрый поиск среди документов по поисковому запросу.

## Использование
> Сервер позволяет обрабатывать любые тексты, даже если они включают служебные символы и множественные пробелы.
### Запуск и наполнение сервера.
0. Для создания сервера используйте следующие конструкторы:
+ SearchServer() - пустой сервер без стоп-слов
+ SearchServer(text_stop_words) - сервер с переданными в текстовом виде стоп-словами с разделителем в виде пробела
+ SearchServer(container) - сервер с переданными в контейнере любого типа стоп-словами
1. Для передачи стоп-слов на сервер используйте метод SetStopWords(text_stop_words), где text - это переданные в текстовом виде стоп-слова с разделителем в виде пробела.
2. Для добавления нового документа используйте метод void AddDocument(document_id, document_text, document_status, document_ratings).
+ document_id: номер документа
+ document_text: текст документа
+ document_status: статус документа, статусы можно посмотреть ниже по тексту.
+ document_ratings: оценки документа, переданные в vector<int>.
3. Для удаления документа используйте метод RemoveDocument(policy, document_id). Policy можно не передавать в запросе, тогда она будет выбрана sequented по умолчанию.
### Получение информации от сервера
0. Для получения ТОП-5 документов по поисковому запросу используйте метод FindTopDocuments() в следующих вариантах:
+ FindTopDocuments(policy, query) - поиск только по документам со статусом ACTUAL, сортировка по убыванию релевантности;
+ FindTopDocuments(policy, query, status) - поиск только по документам со статусом status, сортировка по убыванию релевантности;
+ FindTopDocuments(policy, query, predicate) - поиск по документам, свойства которых описаны в функции predicate. Сортировка по убыванию релевантности или по заданным в predicate условиям.
> Policy можно не передавать в запросе, тогда она будет выбрана sequented по умолчанию.
  
### структура Documents
Найденные по запросу документы отдаются в виде структуры Documents, которая содержит поля:
  + id
  + relevance
  + rating
Данную структуру можно вывести в поток при помощи оператора <<. Так же методом PrintDocument() документ можно вывести в cout.  
### Document's status
  Каждый документ имеет статус. Используйте статусы по своему уСмотрению. Доступные статусы:
  + ACTUAL
  + IRRELEVANT
  + BANNED
  + REMOVED
  
  
## Системные требования
0. С++17(STL)

## Доработки
0. При добавлении документа проработать автоматическое присвоение document_id, если таковое отсутствует.
1. Описать метод MatchDocument.
