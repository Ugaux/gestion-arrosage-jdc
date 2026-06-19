class XIcon extends HTMLElement {
  _lastName = "";

  static get observedAttributes() {
    return ["name"];
  }

  constructor() {
    super();

    this._svg = document.createElementNS("http://www.w3.org/2000/svg", "svg");
    this._use = document.createElementNS("http://www.w3.org/2000/svg", "use");

    this._svg.setAttribute("aria-hidden", "true");
    this._svg.appendChild(this._use);
    this.appendChild(this._svg);
  }

  connectedCallback() {
    this._updateHref();
  }

  attributeChangedCallback(_, oldValue, newValue) {
    if (oldValue === newValue) return;
    this._updateHref();
  }

  _updateHref() {
    const name = this.getAttribute("name");
    if (!name || this._lastName === name) return;

    // safe whether connected or not
    if (this._use) {
      this._use.setAttribute("href", `#${name}`);
      this._lastName = name;
    }
  }
}

export default () => {
  if (!customElements.get("x-icon")) {
    customElements.define("x-icon", XIcon);
  }
};

/* BAD VERSION THAT WAS DOING DOM MANIPULATION EACH TIME
const DEBUG = true;

class XIcon extends HTMLElement {
  static get observedAttributes() {
    return ["name"];
  }

  connectedCallback() {
    if (DEBUG)
      console.log(
        "connected",
        this.getAttribute("name") ?? "(no attribute yet)",
      );
    this.render();
  }

  attributeChangedCallback(name, oldValue, newValue) {
    console.log(this.isConnected);
    if (!this.isConnected) return;
    if (oldValue === newValue) return;
    if (DEBUG)
      console.log(
        "attribute '" + name + "' changed:",
        oldValue,
        "->",
        newValue,
      );

    this.render();
  }

  render() {
    const name = this.getAttribute("name");

    if (!name) return;
    if (DEBUG) console.log("rendering:", name);

    this.innerHTML = `
      <svg aria-hidden="true">
        <use href="#${name}"></use>
      </svg>
    `;
  }
}
*/
