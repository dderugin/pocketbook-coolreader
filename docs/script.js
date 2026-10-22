(() => {
  const button = document.querySelector('#copy-checksum');
  const value = document.querySelector('#checksum-value');
  const status = document.querySelector('#copy-status');
  if (!button || !navigator.clipboard?.writeText || !window.isSecureContext) return;
  button.hidden = false;
  let reset;
  button.addEventListener('click', async () => {
    try {
      await navigator.clipboard.writeText(value.textContent.trim());
      button.querySelector('span').textContent = 'Copied';
      status.textContent = 'SHA-256 checksum copied to clipboard.';
      clearTimeout(reset);
      reset = setTimeout(() => { button.querySelector('span').textContent = 'Copy'; }, 2500);
    } catch {
      status.textContent = 'Copy unavailable. Select the visible checksum to copy it manually.';
      button.querySelector('span').textContent = 'Select hash';
      const range = document.createRange();
      range.selectNodeContents(value);
      window.getSelection().removeAllRanges();
      window.getSelection().addRange(range);
    }
  });
})();
