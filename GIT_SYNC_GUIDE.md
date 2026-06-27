# Guia de Sincronização Git — PC1 ↔ PC2

## Visão Geral

Você trabalha em dois computadores (PC1 e PC2) no mesmo projeto em `C:\Users\Demetrius Mendes\OneDrive\Desktop\DDIno`. O repositório central (origem única da verdade) é o **GitHub** em `https://github.com/demserv/DDIno.git`.

### Fluxo recomendado

```
PC1 ─── git push ──→ GitHub ── git pull ──→ PC2
PC2 ─── git push ──→ GitHub ── git pull ──→ PC1
```

**Regra de ouro:** Sempre faça `git pull` antes de começar a trabalhar, e `git push` ao terminar, em AMBOS os PCs.

---

## Comandos

### 1. Atualizar o repositório local com a versão mais nova do GitHub

Execute SEMPRE antes de começar a trabalhar:

```powershell
# No diretório do projeto:
cd C:\Users\Demetrius Mendes\OneDrive\Desktop\DDIno

# Baixa as alterações do GitHub e aplica sobre seus commits locais
git pull --rebase origin main
```

> `--rebase` evita commits de merge desnecessários. Se houver conflitos, resolva-os editando os arquivos, depois `git add .` e `git rebase --continue`.

### 2. Verificar o que foi modificado (antes de subir)

```powershell
# Mostra arquivos modificados, novos e deletados
git status

# Mostra as diferenças linha a linha
git diff
```

### 3. Adicionar arquivos novos/modificados ao track

```powershell
# Adicionar UM arquivo específico
git add caminho/do/arquivo.ext

# Adicionar TODOS os arquivos (cuidado: pode incluir arquivos grandes ou indesejados)
git add .

# Adicionar apenas arquivos modificados e deletados (não inclui novos)
git add -u

# Ver o que está staged (pronto para commit)
git status
```

### 4. Criar um commit (salva no histórico local)

```powershell
# Comitar com mensagem descritiva
git commit -m "breve descricao do que foi feito"
```

### 5. Subir as alterações para o GitHub

```powershell
# Envia os commits locais para o GitHub (branch main)
git push origin main
```

> Se o push for rejeitado, significa que o GitHub tem commits que você não tem. Faça `git pull --rebase origin main` primeiro e tente novamente.

### 6. Sincronizar o OUTRO PC (exemplo: PC2 após PC1 ter subido)

No outro computador:

```powershell
cd C:\Users\Demetrius Mendes\OneDrive\Desktop\DDIno
git pull --rebase origin main
```

---

## Exemplo de fluxo completo (PC1)

```powershell
# 1. Antes de começar: atualizar
cd C:\Users\Demetrius Mendes\OneDrive\Desktop\DDIno
git pull --rebase origin main

# 2. Trabalhar... editar arquivos, criar novos...

# 3. Ver o que mudou
git status

# 4. Adicionar ao track (exemplo: adiciona tudo)
git add .

# 5. Commitar
git commit -m "Implementa feature X"

# 6. Subir
git push origin main
```

## Exemplo de fluxo completo (PC2)

```powershell
# 1. Puxar o que o PC1 subiu
cd C:\Users\Demetrius Mendes\OneDrive\Desktop\DDIno
git pull --rebase origin main

# 2. Trabalhar...

# 3. Adicionar, commitar, subir
git add .
git commit -m "Ajusta configuracao Y no PC2"
git push origin main
```

---

## Cuidados importantes

| Situação | O que fazer |
|----------|-------------|
| Arquivo grande (>100 MB) | Adicionar ao `.gitignore` antes de `git add .` — o GitHub rejeita arquivos >100 MB |
| Conflito no `git pull --rebase` | Editar arquivos conflitantes, depois `git add .` e `git rebase --continue` |
| Esqueceu de fazer pull antes de trabalhar | Faça `git pull --rebase origin main` — o git reaplica seus commits por cima |
| Push rejeitado | `git pull --rebase origin main` e tente `git push origin main` novamente |
| arquivos compilados/build | Estão no `.gitignore` e não sobem — normal |

---

## Resumo dos comandos essenciais

```powershell
# Atualizar local com o GitHub
git pull --rebase origin main

# Ver status
git status

# Adicionar arquivo ao track
git add nomedoarquivo.ext

# Commitar
git commit -m "mensagem"

# Subir para o GitHub
git push origin main
```

> Use sempre os mesmos comandos no PC1 e no PC2. O GitHub é o ponto de encontro entre eles.
