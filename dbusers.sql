--
-- PostgreSQL database dump
--

SET
statement_timeout = 0;
SET
lock_timeout = 0;
SET
idle_in_transaction_session_timeout = 0;
SET
client_encoding = 'UTF8';
SET
standard_conforming_strings = on;
SELECT pg_catalog.set_config('search_path', '', false);
SET
check_function_bodies = false;
SET
xmloption = content;
SET
client_min_messages = warning;
SET
row_security = off;

--
-- Удаление существующих таблиц и последовательностей
--

-- Сначала удаляем зависимые таблицы
DROP TABLE IF EXISTS public.users CASCADE;

-- Затем удаляем основные таблицы
DROP TABLE IF EXISTS public.backends CASCADE;

-- Удаляем последовательности
DROP SEQUENCE IF EXISTS public.users_id_seq CASCADE;
DROP SEQUENCE IF EXISTS public.backends_id_seq CASCADE;

--
-- Создание новых таблиц
--

SET
default_tablespace = '';
SET
default_table_access_method = heap;

--
-- Создание таблицы backends
--

CREATE TABLE public.backends
(
    id      bigint NOT NULL,
    address character varying,
    region  character varying
);

ALTER TABLE public.backends OWNER TO postgres;

--
-- Создание последовательности для backends.id
--

CREATE SEQUENCE public.backends_id_seq
    START WITH 1
    INCREMENT BY 1
    NO MINVALUE
    NO MAXVALUE CACHE 1;

ALTER TABLE public.backends_id_seq OWNER TO postgres;

--
-- Привязка последовательности к столбцу id
--

ALTER TABLE ONLY public.backends ALTER COLUMN id SET DEFAULT nextval('public.backends_id_seq'::regclass);

--
-- Создание таблицы users
--

CREATE TABLE public.users
(
    id         bigint            NOT NULL,
    login      character varying NOT NULL,
    email      character varying NOT NULL,
    password   character varying NOT NULL,
    backend_id bigint,
    token      character varying,
    token_exp  bigint DEFAULT 0,
    status     bigint DEFAULT 0
);

ALTER TABLE public.users OWNER TO postgres;

--
-- Создание последовательности для users.id
--

CREATE SEQUENCE public.users_id_seq
    START WITH 1
    INCREMENT BY 1
    NO MINVALUE
    NO MAXVALUE CACHE 1;

ALTER TABLE public.users_id_seq OWNER TO postgres;

--
-- Привязка последовательности к столбцу id
--

ALTER TABLE ONLY public.users ALTER COLUMN id SET DEFAULT nextval('public.users_id_seq'::regclass);

--
-- Добавление первичных ключей
--

ALTER TABLE ONLY public.backends
    ADD CONSTRAINT backends_pkey PRIMARY KEY (id);

ALTER TABLE ONLY public.users
    ADD CONSTRAINT users_pkey PRIMARY KEY (id);

--
-- Добавление уникальных индексов
--

CREATE UNIQUE INDEX users_email_key ON public.users USING btree (email);
CREATE UNIQUE INDEX users_login_key ON public.users USING btree (login);
CREATE INDEX users_token_idx ON public.users USING btree (token);

--
-- Добавление внешнего ключа
--

ALTER TABLE ONLY public.users
    ADD CONSTRAINT users_backend_id_fkey FOREIGN KEY (backend_id) REFERENCES public.backends(id);

--
-- Добавление демонстрационных данных
--

INSERT INTO public.backends (id, address, region)
VALUES (1, 'Correct, address', 'eu-west'),
       (2, 'Correct, address2', 'eu-east');

SELECT pg_catalog.setval('public.backends_id_seq', 2, true);

INSERT INTO public.users (id, login, email, password, backend_id, token, token_exp, status)
VALUES (1, 'TestUser0', 'TestUser0@gmail.com', '%passwd123%', 2, NULL, 0, 0),
       (2, 'TestUser1', 'TestUser1@gmail.com', '%passwd123%', 2, NULL, 0, 0);

SELECT pg_catalog.setval('public.users_id_seq', 2, true);

--
-- PostgreSQL database dump complete
--