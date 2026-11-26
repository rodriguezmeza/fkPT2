Local IA tutor
==============

Start an interactive conversation from a source checkout::

   make all
   ./fkpt2 options=IA-tutor

The launcher follows the standalone tutor pattern used in cTreeBalls. It
starts Python before parameter parsing or solver initialization. A linear
spectrum and optional solver addons are unnecessary. Python 3.9 or later is
required. The default uses local Ollama with ``qwen3.5:4b``. The tutor client
and its offline mode use only Python's standard library.

Single questions and machine-readable answers::

   ./fkpt2 options=IA-tutor --ask "How do I compute GSM multipoles in Python?"
   ./fkpt2 options=IA-tutor --ask "What are clpt_b1 and b1?" --json
   ./fkpt2 options=IA-tutor --list-sources --json
   python3 scripts/ia_tutor.py --ask "How do I add a gravity model?"

``make ia-tutor`` starts the Python tutor without building the solver.
``--help`` lists the tutor arguments. Do not combine tutor mode with solver
parameters, other native ``options``, or a multi-process MPI/Slurm launch.

Local Ollama and free Qwen3.5
-----------------------------

Install Ollama using its `official instructions <https://ollama.com/download>`_.
Then download the model and start the local service, unless the Ollama app is
already serving it::

   ollama pull qwen3.5:4b
   ollama serve

In another terminal::

   ./fkpt2 options=IA-tutor
   ./fkpt2 options=IA-tutor --ask "Explain GSM and show Python code for its multipoles"

Switch Ollama off for a launch with ``--backend offline``::

   ./fkpt2 options=IA-tutor --backend offline

During a conversation, ``/backend offline`` disables model calls and
``/backend ollama`` enables them again. To keep Ollama off across launches,
set ``FKPT2_TUTOR_BACKEND=offline`` in your shell environment. An explicit
``--backend ollama`` overrides that preference. ``--model`` or
``FKPT2_TUTOR_MODEL`` selects a different local model.

The `Ollama model library <https://ollama.com/library/qwen3.5>`_ lists local
Qwen3.5 variants, including smaller models for machines with less memory.
Local inference requires no API key or paid inference service. Downloading
the weights requires internet access; model inference runs on your hardware.
Choose an already downloaded model with ``--model``. The tutor does not
download models or start services automatically.

The tutor uses the `Ollama chat API <https://docs.ollama.com/api/chat>`_ to send
the question, retrieved evidence, and a bounded recent conversation. It requests
ordinary answer text with reasoning disabled; ``--think`` enables reasoning.
``--timeout 300`` allows slower machines more time. If the service or model is
unavailable, an explicit warning accompanies offline source excerpts.

``--ollama-url http://127.0.0.1:11434`` changes the local endpoint. Only
loopback addresses are accepted; SSH tunnels to a remote Ollama service work.
HTTP proxies and redirects are disabled, and cloud model tags are rejected.
The model can suggest commands or code, but the tutor has no command-execution
or file-editing tools.

Documentation and PDF coverage
------------------------------

Each launch scans recursive README/changelog files, text documentation under
``doc``, ``docs``, ``documentation``, ``papers``, and ``references`` directories,
reviewed notes under ``tutor/notes``, and ``include/run_parameters.def``.
Markdown, reStructuredText, text, LaTeX, and text manpages are supported.
Generated build/output folders, virtual environments, Git metadata, and
symlinked files/directories are excluded. Source citations identify file and
line ranges; PDFs additionally identify page numbers. ``--list-sources``
reports indexed files and any extraction errors, so incomplete coverage is
visible. Files are reread on launch; no stale text index needs rebuilding.

For reference papers, install either ``pypdf`` in the tutor's Python or the
``pdftotext`` executable (Poppler)::

   python3 -m pip install pypdf

Alternatively, use ``--pdf-python /path/to/python-with-pypdf``. Extracted pages
are cached under ``.fkpt2-tutor/pdf-cache`` by PDF content hash. The cache
allows later offline launches without the extractor. Changed PDFs are
extracted again. Image-only scanned PDFs need OCR; extraction failures are
reported rather than silently counted as indexed papers. Retrieved text is
intended for explanations; inspect the original paper for equations/figures
whose layout does not survive extraction.

Chat history and privacy
------------------------

The development checkout includes private snapshots of the accessible fkPT2
Codex discussions reviewed on 2026-11-26: **Analyze fkPT2 code** and
**Implement 3ptWL emulator in C**, plus the accessible ChatGPT discussion
**Analyze the fkPT paper**. Codex snapshots contain user messages and final
assistant replies, excluding tool outputs and internal reasoning. ChatGPT
snapshots contain the user/assistant text returned by the history service;
attachment contents are not included. This is a
snapshot of accessible project chats, not access to every past ChatGPT account
conversation or a live connection to later chats.

Personal snapshots, imports, PDF caches, and saved conversations stay under
the Git-ignored ``.fkpt2-tutor`` directory and are excluded from source packages.
Only the reviewed project summary under ``tutor/notes`` is distributed.
Import more conversations as needed::

   ./fkpt2 options=IA-tutor --import-chat /path/to/conversations.json
   ./fkpt2 options=IA-tutor --import-chat /path/to/fkpt2-discussion.md
   ./fkpt2 options=IA-tutor --chat-dir /path/to/other-local-chats

JSON supports ChatGPT ``conversations.json`` mappings, message arrays with
``role`` and ``content``, and the tutor's own snapshots. Imports normalize
user/assistant text and omit system/tool records. Text transcripts may be
``.md``, ``.rst``, or ``.txt``. Import files can contain multiple conversations;
``--import-chat`` and ``--chat-dir`` can be repeated. Imports are copied into
the private corpus; original files are preserved. Imported text can be sent
as evidence to the local model when Ollama is selected. ``--no-chats`` excludes
both private transcripts and reviewed historical notes from a launch.

Current documentation receives higher retrieval weight than historical chat
claims. Every historical source is labelled in the answer's citation list.
Offline answers are explicitly extracted passages; Ollama optionally
synthesizes an explanation. Source citations make either mode reviewable.

Conversation commands and configuration
---------------------------------------

``/help`` lists commands. ``/sources`` shows coverage; ``/backend offline`` and
``/backend ollama`` switch response mode; ``/model qwen3.5:4b`` selects a model.
``/reset`` clears the current conversation. ``/save`` explicitly saves it for
retrieval on a later launch. ``/quit``, ``/exit``, or Ctrl-D exits. Conversations
are otherwise held only in memory. Short follow-ups use the preceding question
for retrieval and a bounded recent history for model context.

Environment defaults, overridden by command-line values:

.. list-table::
   :header-rows: 1

   * - Variable
     - Purpose
   * - ``FKPT2_TUTOR_PYTHON``
     - Native launcher's Python executable (one path, no shell arguments)
   * - ``FKPT2_TUTOR_ROOT``
     - Source checkout for launcher/resource discovery
   * - ``FKPT2_TUTOR_BACKEND``
     - ``ollama`` (default) or ``offline`` to disable model calls
   * - ``FKPT2_TUTOR_MODEL``
     - Local model tag, default ``qwen3.5:4b``
   * - ``FKPT2_TUTOR_OLLAMA_URL``
     - Local base URL, default ``http://127.0.0.1:11434``
   * - ``FKPT2_TUTOR_PDF_PYTHON``
     - Optional Python executable containing ``pypdf``

The native launcher locates resources beside its executable, in ancestor
directories, or at the compiled source path. Set ``FKPT2_TUTOR_ROOT`` when
moving a binary away from its checkout. A binary or Python wheel alone does
not contain the documentation corpus; use a source checkout or source archive.
``make test-ia-tutor`` verifies retrieval, chat imports, native launch behavior,
and the Ollama protocol using a mock local response without downloading a model.
